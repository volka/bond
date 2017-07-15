$env:PreferredToolArchitecture = "x64"
$env:_IsNativeEnvironment = "true"

if ($env:BOND_BUILD -eq "Doc") {
    cmake --build . --target documentation -- /verbosity:minimal /logger:"C:\Program Files\AppVeyor\BuildAgent\Appveyor.MSBuildLogger.dll"

    if ($? -And $env:BOND_TOKEN -And $env:APPVEYOR_REPO_BRANCH -eq "master") {
        git config --global user.email "bondlab@microsoft.com"
        git config --global user.name "Appveyor"
        git clone -b gh-pages "https://${env:BOND_TOKEN}@github.com/Microsoft/bond.git" gh-pages 2>&1 | out-null

        cd gh-pages

        if (-not $?) { throw "Cloning gh-pages failed" }

        Remove-Item * -Recurse
        Copy-Item ..\html\* . -Recurse
        git add --all .
        git commit -m "Update documentation"
        git push origin gh-pages 2>&1 | out-null

        cd ..
    }
}

if ($env:BOND_BUILD -eq "Python") {
    cmake --build . --target python_unit_test -- /verbosity:minimal /logger:"C:\Program Files\AppVeyor\BuildAgent\Appveyor.MSBuildLogger.dll"
}

if ($env:BOND_BUILD -eq "C++") {
    cmake --build . --target check -- /verbosity:minimal /logger:"C:\Program Files\AppVeyor\BuildAgent\Appveyor.MSBuildLogger.dll"
}

if ($env:BOND_BUILD -eq "C#") {
    msbuild cs\cs.sln /verbosity:minimal /p:Configuration=${env:BOND_CONFIG} /logger:"C:\Program Files\AppVeyor\BuildAgent\Appveyor.MSBuildLogger.dll"
}

if ($env:BOND_BUILD -eq "C# .NET Core") {
    & cs\dnc\build.ps1 -Configuration $env:BOND_CONFIG -Verbosity minimal -MSBuildLogger "C:\Program Files\AppVeyor\BuildAgent\Appveyor.MSBuildLogger.dll"
}

if (-not $?) { throw "build failed" }
