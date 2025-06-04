// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

package org.bondlib;

import com.fasterxml.jackson.core.*;

import java.io.IOException;
import java.io.OutputStream;

/**
 * Implements Simple JSON protocol writer for object serialization.
 */
public final class SimpleJsonWriter implements ProtocolWriter {

    private final JsonGenerator generator;
    private final java.util.Stack<Boolean> containerTypeStack = new java.util.Stack<>();
    private boolean expectingMapKey = false;

    public SimpleJsonWriter(OutputStream outputStream) throws IOException {
        ArgumentHelper.ensureNotNull(outputStream, "outputStream");
        this.generator = JsonGlobals.jsonFactory.createGenerator(outputStream);
    }

    @Override
    public final boolean usesMarshaledBonded() {
        return false;
    }

    @Override
    public final void writeVersion() throws IOException {
        // this protocol does not support marshalling
    }

    @Override
    public final void writeStructBegin(Metadata metadata) throws IOException {
        this.generator.writeStartObject();
    }

    @Override
    public final void writeStructEnd() throws IOException {
        this.generator.writeEndObject();
    }

    @Override
    public final void writeBaseBegin(Metadata metadata) throws IOException {
        // this protocol flattens type hierarchy
    }

    @Override
    public final void writeBaseEnd() throws IOException {
        // this protocol flattens type hierarchy
    }

    @Override
    public final void writeFieldBegin(BondDataType bondDataType, int i, Metadata metadata) throws IOException {
        this.generator.writeFieldName(metadata.name);
    }

    @Override
    public final void writeFieldEnd() throws IOException {
    }

    @Override
    public final void writeFieldOmitted(BondDataType bondDataType, int i, Metadata metadata) throws IOException {
    }

    @Override
    public final void writeContainerBegin(int i, BondDataType bondDataType) throws IOException {
        // Non-map container (list, set, etc.)
        this.containerTypeStack.push(false);
        this.generator.writeStartArray();
    }

    @Override
    public final void writeContainerBegin(int i, BondDataType bondDataType, BondDataType bondDataType1) throws IOException {
        // Map container - use JSON objects instead of arrays
        this.containerTypeStack.push(true);
        this.generator.writeStartObject();
        this.expectingMapKey = true;
    }

    @Override
    public void writeContainerEnd() throws IOException {
        boolean isMap = this.containerTypeStack.pop();
        if (isMap) {
            this.generator.writeEndObject();
            this.expectingMapKey = false;
        } else {
            this.generator.writeEndArray();
        }
    }

    private void writeMapKeyOrValue(String value) throws IOException {
        if (this.expectingMapKey) {
            this.generator.writeFieldName(value);
            this.expectingMapKey = false;
        } else {
            this.generator.writeString(value);
            this.expectingMapKey = true;
        }
    }

    private void writeMapKeyOrValue(Number value) throws IOException {
        if (this.expectingMapKey) {
            this.generator.writeFieldName(value.toString());
            this.expectingMapKey = false;
        } else {
            this.generator.writeNumber(value.toString());
            this.expectingMapKey = true;
        }
    }

    private void writeMapKeyOrValue(boolean value) throws IOException {
        if (this.expectingMapKey) {
            this.generator.writeFieldName(Boolean.toString(value));
            this.expectingMapKey = false;
        } else {
            this.generator.writeBoolean(value);
            this.expectingMapKey = true;
        }
    }

    @Override
    public final void writeInt8(byte b) throws IOException {
        if (this.expectingMapKey || (!this.containerTypeStack.isEmpty() && this.containerTypeStack.peek())) {
            this.writeMapKeyOrValue(Byte.valueOf(b));
        } else {
            this.generator.writeNumber(b);
        }
    }

    @Override
    public final void writeInt16(short i) throws IOException {
        if (this.expectingMapKey || (!this.containerTypeStack.isEmpty() && this.containerTypeStack.peek())) {
            this.writeMapKeyOrValue(Short.valueOf(i));
        } else {
            this.generator.writeNumber(i);
        }
    }

    @Override
    public final void writeInt32(int i) throws IOException {
        if (this.expectingMapKey || (!this.containerTypeStack.isEmpty() && this.containerTypeStack.peek())) {
            this.writeMapKeyOrValue(Integer.valueOf(i));
        } else {
            this.generator.writeNumber(i);
        }
    }

    @Override
    public final void writeInt64(long l) throws IOException {
        if (this.expectingMapKey || (!this.containerTypeStack.isEmpty() && this.containerTypeStack.peek())) {
            this.writeMapKeyOrValue(Long.valueOf(l));
        } else {
            this.generator.writeNumber(l);
        }
    }

    @Override
    public final void writeUInt8(byte b) throws IOException {
        if (this.expectingMapKey || (!this.containerTypeStack.isEmpty() && this.containerTypeStack.peek())) {
            this.writeMapKeyOrValue(UnsignedHelper.asUnsignedShort(b));
        } else {
            this.generator.writeNumber(UnsignedHelper.asUnsignedShort(b));
        }
    }

    @Override
    public final void writeUInt16(short i) throws IOException {
        if (this.expectingMapKey || (!this.containerTypeStack.isEmpty() && this.containerTypeStack.peek())) {
            this.writeMapKeyOrValue(UnsignedHelper.asUnsignedInt(i));
        } else {
            this.generator.writeNumber(UnsignedHelper.asUnsignedInt(i));
        }
    }

    @Override
    public final void writeUInt32(int i) throws IOException {
        if (this.expectingMapKey || (!this.containerTypeStack.isEmpty() && this.containerTypeStack.peek())) {
            this.writeMapKeyOrValue(UnsignedHelper.asUnsignedLong(i));
        } else {
            this.generator.writeNumber(UnsignedHelper.asUnsignedLong(i));
        }
    }

    @Override
    public final void writeUInt64(long l) throws IOException {
        if (this.expectingMapKey || (!this.containerTypeStack.isEmpty() && this.containerTypeStack.peek())) {
            this.writeMapKeyOrValue(UnsignedHelper.asUnsignedBigInt(l));
        } else {
            this.generator.writeNumber(UnsignedHelper.asUnsignedBigInt(l));
        }
    }

    @Override
    public final void writeFloat(float v) throws IOException {
        if (this.expectingMapKey || (!this.containerTypeStack.isEmpty() && this.containerTypeStack.peek())) {
            this.writeMapKeyOrValue(Float.valueOf(v));
        } else {
            this.generator.writeNumber(v);
        }
    }

    @Override
    public final void writeDouble(double v) throws IOException {
        if (this.expectingMapKey || (!this.containerTypeStack.isEmpty() && this.containerTypeStack.peek())) {
            this.writeMapKeyOrValue(Double.valueOf(v));
        } else {
            this.generator.writeNumber(v);
        }
    }

    @Override
    public final void writeBytes(byte[] bytes) throws IOException {
        // Bytes are always written as arrays, even in maps
        this.generator.writeStartArray();
        for (int i = 0; i < bytes.length; ++i) {
            this.generator.writeNumber(bytes[i]);
        }
        this.generator.writeEndArray();
        if (this.expectingMapKey) {
            this.expectingMapKey = false;
        } else if (!this.containerTypeStack.isEmpty() && this.containerTypeStack.peek()) {
            this.expectingMapKey = true;
        }
    }

    @Override
    public final void writeBool(boolean b) throws IOException {
        if (this.expectingMapKey || (!this.containerTypeStack.isEmpty() && this.containerTypeStack.peek())) {
            this.writeMapKeyOrValue(b);
        } else {
            this.generator.writeBoolean(b);
        }
    }

    @Override
    public final void writeString(String s) throws IOException {
        if (this.expectingMapKey || (!this.containerTypeStack.isEmpty() && this.containerTypeStack.peek())) {
            this.writeMapKeyOrValue(s);
        } else {
            this.generator.writeString(s);
        }
    }

    @Override
    public final void writeWString(String s) throws IOException {
        if (this.expectingMapKey || (!this.containerTypeStack.isEmpty() && this.containerTypeStack.peek())) {
            this.writeMapKeyOrValue(s);
        } else {
            this.generator.writeString(s);
        }
    }
}
