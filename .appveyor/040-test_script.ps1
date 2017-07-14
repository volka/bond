            if ($env:BOND_BUILD -eq "Python") {

                ctest -C Debug --tests-regex python_unit_test --output-on-failure
                if (-not $?) { throw "tests failed" }

            }

            if ($env:BOND_BUILD -eq "C#") {

                nunit-console-x86 /framework:net-4.5 /labels "cs\test\core\bin\debug\net45\${env:BOND_OUTPUT}\Bond.UnitTest.dll" cs\test\internal\bin\debug\net45\Bond.InternalTest.dll
                if (-not $?) { throw "tests failed" }

                nunit-console-x86 /framework:net-4.5 /labels "cs\test\core\bin\debug\net45-nonportable\${env:BOND_OUTPUT}\Bond.UnitTest.dll"
                if (-not $?) { throw "tests failed" }

                & examples\cs\grpc\pingpong\bin\Debug\pingpong.exe
                if (-not $?) { throw "tests failed" }

                # We need to investigate why these tests are failing in AppVeyor, but not locally.
                # nunit-console-x86 /framework:net-4.5 /labels "cs\test\comm\bin\debug\net45\${env:BOND_OUTPUT}\Bond.Comm.UnitTest.dll"
                # if (-not $?) { throw "tests failed" }

            }

            if ($env:BOND_BUILD -eq "C# .NET Core") {

                & cs\dnc\build.ps1 -Test -Configuration $env:BOND_CONFIG -Verbosity minimal -MSBuildLogger "C:\Program Files\AppVeyor\BuildAgent\Appveyor.MSBuildLogger.dll"
                if (-not $?) { throw "tests failed" }

            }

            if (-not $?) { throw "build failed" }
