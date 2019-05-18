// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifdef _MSC_VER
    #pragma warning (push)
    // warning C4100: unreferenced formal parameter
    //
    // warning C4127: conditional expression is constant
    //
    // warning C4505: unreferenced local function has been removed
    //
    // warning C4800: 'int': forcing value to bool 'true' or 'false' (performance warning)
    #pragma warning (disable: 4100 4127 4505 4800)
#endif

#include <grpc/grpc.h>
#include <grpc/support/time.h>
#include <grpcpp/alarm.h>
#include <grpcpp/grpcpp.h>
#include <grpcpp/impl/codegen/completion_queue.h>
#include <grpcpp/impl/grpc_library.h>

#ifdef _MSC_VER
    #pragma warning(pop)
#endif

#include <bond/ext/grpc/io_manager.h>
#include <bond/ext/grpc/detail/io_manager_tag.h>

// TODO: move unit_test_framework.h to cpp/test/inc
#include "../core/unit_test_framework.h"
#include "barrier.h"
#include "countdown_event.h"
#include "event.h"

#include <boost/chrono.hpp>
#include <boost/test/debug.hpp>
#include <atomic>
#include <memory>
#include <utility>

using namespace bond::ext::grpc::detail;
using namespace bond::ext::grpc;

template <typename TEvent>
struct alarm_completion_tag : io_manager_tag
{
    TEvent completion_event;

    template <typename... TArg>
    explicit alarm_completion_tag(TArg&&... args)
        : completion_event(std::forward<TArg>(args)...)
    { }

    void invoke(bool) override
    {
        completion_event.set();
    }
};

class io_managerTests
{
    static void PollOneItem()
    {
        io_manager ioManager;

        alarm_completion_tag<unit_test::event> act;
        gpr_timespec deadline = gpr_time_0(GPR_CLOCK_MONOTONIC);
        ::grpc::Alarm alarm(ioManager.cq(), deadline, act.tag());

        bool wasSet = act.completion_event.wait_for(std::chrono::seconds(30));
        UT_AssertIsTrue(wasSet);
    }

    static void PollManyItems()
    {
        io_manager ioManager;

        const size_t numItems = 1000;

        alarm_completion_tag<unit_test::countdown_event> act(numItems);

        const gpr_timespec deadline = gpr_time_0(GPR_CLOCK_MONOTONIC);

        std::vector<::grpc::Alarm> alarms;
        alarms.reserve(numItems);
        for (size_t i = 0; i < numItems; ++i)
        {
            alarms.emplace_back(ioManager.cq(), deadline, act.tag());
        }

        bool wasSet = act.completion_event.wait_for(std::chrono::seconds(30));
        UT_AssertIsTrue(wasSet);
    }

    static void ShutdownUnstarted()
    {
        io_manager ioManager(1, true);
        ioManager.shutdown();
        ioManager.wait();

        // also tests that we can run the dtor after successful shutdown
    }

    static void ConcurrentShutdown()
    {
        io_manager ioManager(
            1,
            false,
            // also tests that we can pass an explicit completion queue
            std::unique_ptr<::grpc::CompletionQueue>(new ::grpc::CompletionQueue));

        const size_t numConcurrentShutdowns = 5;
        unit_test::barrier threadsStarted(numConcurrentShutdowns);
        unit_test::barrier threadsObservedShutdown(numConcurrentShutdowns);

        std::vector<std::thread> threads;
        threads.reserve(5);
        for (size_t i = 0; i < numConcurrentShutdowns; ++i)
        {
            threads.emplace_back([&ioManager, &threadsStarted, &threadsObservedShutdown]()
            {
                threadsStarted.enter();

                ioManager.shutdown();
                ioManager.wait();

                threadsObservedShutdown.enter();
            });
        }

        bool wasSet = threadsStarted.wait_for(std::chrono::seconds(30));
        UT_AssertIsTrue(wasSet); // all the threads took too long to get started

        wasSet = threadsObservedShutdown.wait_for(std::chrono::seconds(30));
        UT_AssertIsTrue(wasSet); // took too long to see the io_manager shutdown

        for (auto& thread : threads)
        {
            thread.join();
        }
    }

    static void DelayStartDoesntStart()
    {
        io_manager ioManager(
            1,
            true,
            // also tests that we can pass an explicit completion queue
            std::unique_ptr<::grpc::CompletionQueue>(new ::grpc::CompletionQueue));

        alarm_completion_tag<unit_test::event> act;
        gpr_timespec deadline = gpr_time_0(GPR_CLOCK_MONOTONIC);
        ::grpc::Alarm alarm(ioManager.cq(), deadline, act.tag());

        bool wasSet = act.completion_event.wait_for(std::chrono::milliseconds(1250));
        UT_AssertIsTrue(!wasSet);

        // since we've put something into the completion queue, we need to
        // start it so that it can be drained and shutdown.
        ioManager.start();

        // test that we can call start multiple times
        ioManager.start();

        wasSet = act.completion_event.wait_for(std::chrono::seconds(30));
        UT_AssertIsTrue(wasSet);
    }

public:
    static void Initialize()
    {
        UnitTestSuite suite("io_manager");
        suite.AddTestCase(PollOneItem, "Poll one item");
        suite.AddTestCase(PollManyItems, "Poll many items");
        suite.AddTestCase(ShutdownUnstarted, "Shutdown unstarted");
        suite.AddTestCase(ConcurrentShutdown, "Concurrent shutdown");
        suite.AddTestCase(DelayStartDoesntStart, "Delay Start doesn't start");
    }
};

bool init_unit_test()
{
    io_managerTests::Initialize();
    return true;
}
