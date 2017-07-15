if (($env:BOND_BUILD -eq 'C++') -or ($env:BOND_BUILD -eq 'Python')) {
    git submodule update --init thirdparty\rapidjson

    # If gRPC is not disabled, init gRPC and its dependencies
    if (($env:BOND_BUILD -eq 'C++') -and (-not ($env:BOND_CMAKE_FLAGS -match '-DBOND_ENABLE_GRPC=FALSE'))) {
        git submodule update --init --recursive thirdparty\grpc
    }
}

if ($env:BOND_BOOST -eq 56) {
    # Hard-coded Boost path per https://www.appveyor.com/docs/installed-software#languages-libraries-frameworks
    $env:BOOST_ROOT = "C:/Libraries/boost"
    $env:BOOST_LIBRARYDIR = "C:/Libraries/boost/lib${env:BOND_ARCH}-msvc-${env:BOND_VS_NUM}.0"
} else {
    $env:BOOST_ROOT = "C:/Libraries/boost_1_${env:BOND_BOOST}_0"
    $env:BOOST_LIBRARYDIR = "C:/Libraries/boost_1_${env:BOND_BOOST}_0/lib${env:BOND_ARCH}-msvc-${env:BOND_VS_NUM}.0"
}

choco install haskell-stack -y

# choco install updated the path, so re-read them from the registry and reset $env:path
$machinePath = [System.Environment]::GetEnvironmentVariable("Path","Machine")
$userPath = [System.Environment]::GetEnvironmentVariable("Path","User")
$env:Path = "$machinePath;$userPath"

if ($env:BOND_BUILD -eq "Doc") {
    choco install pandoc --version 1.19.2  -y
    choco install doxygen.install -y

    $machinePath = [System.Environment]::GetEnvironmentVariable("Path","Machine")
    $userPath = [System.Environment]::GetEnvironmentVariable("Path","User")
    $env:Path = "$machinePath;$userPath"
}

if (($env:BOND_BUILD -eq 'C++') -and (-not ($env:BOND_CMAKE_FLAGS -match '-DBOND_ENABLE_GRPC=FALSE'))) {
    # We're building C++ and gRPC isn't disabled, so we need
    # gRPC dependencies.
    choco install yasm -y

    $machinePath = [System.Environment]::GetEnvironmentVariable("Path","Machine")
    $userPath = [System.Environment]::GetEnvironmentVariable("Path","User")
    $env:Path = "$machinePath;$userPath"
}
