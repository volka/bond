            $env:PreferredToolArchitecture = "x64"

            $env:_IsNativeEnvironment = "true"

            $cmakeGenerator = $env:BOND_VS

            if ($env:BOND_ARCH -eq 64) {
                $cmakeGenerator += " Win64"
            }

            $cmakeFlags = $env:BOND_CMAKE_FLAGS -split ';'

            if ($env:BOND_BUILD -eq "C#" -Or $env:BOND_BUILD -eq "C++") {

                nuget restore cs\cs.sln

            }

            if ($env:BOND_BUILD -eq "Python" -Or $env:BOND_BUILD -eq "C++") {

                # Make sure we have Python27-64 before any other version

                if ($env:BOND_ARCH -eq 64) {
                    $env:Path = "C:\Python27-x64\scripts;C:\Python27-x64\;${env:Path}"
                }

            }

            if ($env:BOND_BUILD -eq "Python") {

                mkdir build

                cd build

                cmake "-DBoost_ADDITIONAL_VERSIONS=1.${env:BOND_BOOST}.0" $cmakeFlags -G $cmakeGenerator ..

            }

            if ($env:BOND_BUILD -eq "C++") {

                # We don't always need all of these compat tests--depending
                # on what part of C++ we're building--but they're pretty
                # fast to build, so build them all.

                $compatTests = ('Tests\CommCompatClient', 'Tests\CommCompatServer', 'Tests\Compat', 'Tests\GrpcCompatClient', 'Tests\GrpcCompatServer')

                msbuild cs\cs.sln /verbosity:minimal "/target:$($compatTests -join ';')" /logger:"C:\Program Files\AppVeyor\BuildAgent\Appveyor.MSBuildLogger.dll"

                if (-not $?) { throw "cs compat build failed" }

                mkdir build

                cd build

                cmake $cmakeFlags -G $cmakeGenerator .. 2>cmake_stderr.log

                Get-Content cmake_stderr.log

            }

            if ($env:BOND_BUILD -eq "Doc") {

                mkdir build

                cd build

                cmake ../doc

            }