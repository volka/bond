# Security review

Scope: repository-level review with emphasis on the C++ implementation, especially the Simple JSON protocol. This is a source review, not a full audit or exploit development exercise.

## Executive summary

- The largest practical risk is denial of service from untrusted input, especially on the JSON path: full-DOM parsing, permissive parsing of extra data, and repeated linear scans over object members make CPU and memory exhaustion realistic.
- The general C++ codebase contains several low-level, performance-oriented paths that are carefully bounded in many places, but they remain the areas that deserve the most fuzzing and portability review.
- The project is end-of-life, and `SECURITY.md` states there will be no further security fixes. That increases the importance of downstream hardening and dependency review.

## Highest-priority findings

### 1. Simple JSON accepts concatenated payloads unless the caller enforces framing

**Why it matters:** message smuggling / parser differentials / trailing-data confusion.

- `SimpleJsonReader::Parse` uses RapidJSON with `kParseStopWhenDoneFlag`, so parsing stops after one complete JSON value instead of requiring end-of-input: `cpp/inc/bond/protocol/simple_json_reader.h:68-75`.
- The tests explicitly rely on this behavior and deserialize multiple JSON objects from one buffer/stream: `cpp/test/core/json_tests.cpp:52-104`, `cpp/test/core/json_tests.cpp:202-207`, `cpp/test/core/json_tests.cpp:354-358`.

**Risk:** if an application assumes “one input buffer == one message”, an attacker may append additional JSON values or garbage after the first valid document. Bond can legally consume only the first document and leave the remainder for later processing.

**Recommendation:** treat transport framing / EOF checks as mandatory for untrusted JSON, and verify that every caller drains the input exactly as expected.

### 2. Simple JSON is vulnerable to memory/CPU DoS before schema-level recursion limits help

**Why it matters:** attacker-controlled payloads can force large allocations and deep parse work even when the application schema ignores most of the JSON.

- `SimpleJsonReader::Parse` always builds a full `rapidjson::Document`: `cpp/inc/bond/protocol/simple_json_reader.h:63-76`.
- The deep recursion guard is applied during deserialization, not before DOM construction: `cpp/inc/bond/core/parser.h:448-455`, `cpp/inc/bond/core/detail/recursionguard.h:31-56`.
- Tests show that payloads with 10,000 nested arrays or 10,000 nested objects are accepted for a simple target type: `cpp/test/core/json_tests.cpp:228-273`.
- JSON containers are sized directly from `reader.ArraySize()` with no explicit application-level cap: `cpp/inc/bond/protocol/simple_json_reader_impl.h:55-58`, `cpp/inc/bond/protocol/simple_json_reader_impl.h:98-101`.

**Risk:** very large or deeply nested JSON can consume memory and CPU during parse and container materialization even if later business logic does little with the data.

**Recommendation:** impose external request-size limits, consider allocator caps for untrusted workloads, and fuzz large/deep JSON inputs specifically.

### 3. Simple JSON field lookup is algorithmically expensive and duplicate-key handling is ambiguous

**Why it matters:** CPU amplification and inconsistent interpretation of malicious JSON objects.

- For compile-time schemas, the parser calls `FindField` once per field: `cpp/inc/bond/core/parser.h:470-486`.
- For runtime schemas, it does the same over `schema.GetStruct().fields`: `cpp/inc/bond/core/parser.h:495-522`.
- `FindField` scans every member of the JSON object linearly and returns the first match by field name or numeric string id: `cpp/inc/bond/protocol/simple_json_reader_impl.h:16-46`.

**Risk:**

- Large objects with many unused members can force repeated O(schema_fields × object_members) work.
- Duplicate keys are not rejected; the first matching entry wins. That is dangerous whenever another component in the pipeline uses different duplicate-key semantics.
- Accepting both metadata names and numeric ids broadens the input surface and should be treated as another ambiguity source.

**Recommendation:** fuzz with large objects and duplicate keys, and treat duplicate-key policy as a required integration check.

### 4. Simple JSON deserialization is permissive in ways that can hide malformed input

**Why it matters:** partial parsing can turn attacker-controlled malformed data into defaulted or silently dropped values instead of a hard failure.

- List deserialization pre-sizes the output and silently advances past elements whose JSON type does not match: `cpp/inc/bond/protocol/simple_json_reader_impl.h:98-117`.
- Set deserialization ignores elements whose type does not match: `cpp/inc/bond/protocol/simple_json_reader_impl.h:135-142`.
- Blob deserialization copies only integer elements and silently drops the rest: `cpp/inc/bond/protocol/simple_json_reader_impl.h:71-83`.

**Risk:** this is a weak spot for validation bypasses when applications assume deserialization is strict. A malicious payload may become a partially defaulted object rather than an error.

**Recommendation:** any security-sensitive caller should validate required invariants after deserialize, not assume malformed JSON was rejected.

## General C++ findings

### 5. `SetDeserializeMaxDepth` is process-global mutable state

**Why it matters:** security policy can change across threads or requests.

- The recursion limit is stored in shared static state: `cpp/inc/bond/core/detail/recursionguard.h:16-29`.
- `currentDepth` is `thread_local`, but `maxDepth` is not: `cpp/inc/bond/core/detail/recursionguard.h:18-23`.
- The public setter mutates that shared limit globally: `cpp/inc/bond/core/bond.h:131-135`.
- Tests also mutate it globally: `cpp/test/core/json_tests.cpp:318-345`.

**Risk:** one component can unintentionally or maliciously weaken/strengthen the deserialize depth policy for other concurrent work.

**Recommendation:** avoid changing this at runtime in multi-tenant or multi-threaded services without external synchronization.

### 6. `capped_allocator` accounting uses unchecked multiplication

**Why it matters:** a memory cap should be a hard boundary if it is used as a defense against hostile payloads.

- `allocate` computes `n * sizeof(value_type)` before calling `try_add`, without checked arithmetic: `cpp/inc/bond/ext/capped_allocator.h:155-173`.
- The same pattern appears in the hinted overload and in deallocation accounting: `cpp/inc/bond/ext/capped_allocator.h:175-203`.

**Risk:** with sufficiently large `n`, the multiplication can wrap in `size_type`, undercount the requested bytes, and weaken the cap. Real exploitability depends on container behavior and the underlying allocator, but this should not be trusted as a strict guardrail without deeper testing.

**Recommendation:** use checked multiplication here before relying on allocator caps as a security boundary.

### 7. The repository contains low-level raw memory fast paths that deserve focused fuzzing

These are not obvious bugs from inspection, but they are the highest-risk implementation areas in the general C++ code:

- `InputBuffer::Read` uses `reinterpret_cast<const T*>` fast paths on x86/x64 after bounds checks: `cpp/inc/bond/stream/input_buffer.h:138-163`.
- `OutputBuffer::Write` uses `reinterpret_cast<T*>` fast paths on x86/x64 before falling back to `memcpy`: `cpp/inc/bond/stream/output_buffer.h:182-209`.
- The codebase also uses manual copying and pointer arithmetic in several foundational components: `cpp/inc/bond/core/detail/checked.h:21-59`, `cpp/inc/bond/protocol/detail/simple_array.h:70-82`.

**Risk:** these paths are performance-oriented and central. They deserve the most compiler/architecture matrix testing, sanitizer coverage, and fuzzing, especially if the library is built outside the originally expected platforms.

## Positive controls already present

These do not eliminate the findings above, but they are valuable hardening points:

- Binary readers explicitly check string sizes and use checked arithmetic before reading variable-size content: `cpp/inc/bond/protocol/simple_binary.h:165-176`, `cpp/inc/bond/protocol/simple_binary.h:186-225`, `cpp/inc/bond/protocol/compact_binary.h:375-414`, `cpp/inc/bond/protocol/fast_binary.h:187-218`.
- The parser uses a recursion guard during deserialize: `cpp/inc/bond/core/parser.h:448-455`, `cpp/inc/bond/core/detail/recursionguard.h:31-56`.
- Wide-string conversion on the JSON path uses strict UTF conversion and throws on invalid input: `cpp/inc/bond/protocol/detail/rapidjson_utils_impl.h:17-28`, `cpp/inc/bond/protocol/detail/rapidjson_helper.h:281-296`.

## Dependency and lifecycle risk

- `SECURITY.md` states the project has ended and that there will be no security fixes: `SECURITY.md:1-8`.
- RapidJSON is carried as a submodule dependency: `.gitmodules:1-3`.

**Risk:** even if the local code is acceptable for your threat model, downstream users inherit long-term exposure from an unmaintained codebase and third-party dependency set.

## Suggested deeper-audit targets

1. Fuzz the JSON reader with:
   - duplicate keys
   - mixed name/id addressing
   - concatenated documents
   - very large arrays/objects
   - deeply nested ignored subtrees
2. Review every production caller that deserializes untrusted JSON and confirm:
   - framing/EOF is enforced
   - request-size limits exist
   - post-deserialize validation exists
3. Stress-test `capped_allocator` with extreme `n` values and sanitizers.
4. Run sanitizers/fuzzers against the stream and protocol primitives, especially `InputBuffer`, `OutputBuffer`, and JSON container parsing.

## Bottom line

If this library processes untrusted input, the JSON path should be treated as the highest-risk surface for practical attacks, mainly denial of service and parser differential issues. Outside JSON, the main concerns are global mutable deserialize policy, cap-accounting edge cases, and reliance on low-level memory primitives in a now-unmaintained C++ codebase.
