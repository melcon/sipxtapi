@echo off

set SYNCHRONIZER_JAR=contrib\VSCppProjectSynchronizer\VSCppProjectSynchronizer.jar

echo Synchronizing VS2005 project files with VS2003...

rem Synchronize sipXcallLib and sipXtapi projects
java -jar %SYNCHRONIZER_JAR% sipXcallLib\sipXcallLib-msvc8.vcproj sipXcallLib\sipXcallLib-msvc71.vcproj
java -jar %SYNCHRONIZER_JAR% sipXcallLib\sipXcallLibTest-msvc8.vcproj sipXcallLib\sipXcallLibTest-msvc71.vcproj
java -jar %SYNCHRONIZER_JAR% sipXcallLib\sipXtapi-msvc8.vcproj sipXcallLib\sipXtapi-msvc71.vcproj
java -jar %SYNCHRONIZER_JAR% sipXcallLib\sipXtapiTest-msvc8.vcproj sipXcallLib\sipXtapiTest-msvc71.vcproj

rem Synchronize sipXmediaAdapterLib projects
java -jar %SYNCHRONIZER_JAR% sipXmediaAdapterLib\sipXmediaAdapterLib-msvc8.vcproj sipXmediaAdapterLib\sipXmediaAdapterLib-msvc71.vcproj
java -jar %SYNCHRONIZER_JAR% sipXmediaAdapterLib\sipXmediaAdapterLibTest-msvc8.vcproj sipXmediaAdapterLib\sipXmediaAdapterLibTest-msvc71.vcproj

rem Synchronize sipXmediaLib projects
java -jar %SYNCHRONIZER_JAR% sipXmediaLib\sipXmediaLib-msvc8.vcproj sipXmediaLib\sipXmediaLib-msvc71.vcproj
java -jar %SYNCHRONIZER_JAR% sipXmediaLib\sipXmediaLibTest-msvc8.vcproj sipXmediaLib\sipXmediaLibTest-msvc71.vcproj

rem Synchronize sipXportLib projects
java -jar %SYNCHRONIZER_JAR% sipXportLib\sipXportLib-msvc8.vcproj sipXportLib\sipXportLib-msvc71.vcproj
java -jar %SYNCHRONIZER_JAR% sipXportLib\sipXportLibTest-msvc8.vcproj sipXportLib\sipXportLibTest-msvc71.vcproj
java -jar %SYNCHRONIZER_JAR% sipXportLib\sipXunitTestSuite-msvc8.vcproj sipXportLib\sipXunitTestSuite-msvc71.vcproj

rem Synchronize sipXsdpLib projects
java -jar %SYNCHRONIZER_JAR% sipXsdpLib\sipXsdpLib-msvc8.vcproj sipXsdpLib\sipXsdpLib-msvc71.vcproj
java -jar %SYNCHRONIZER_JAR% sipXsdpLib\sipXsdpLibTest-msvc8.vcproj sipXsdpLib\sipXsdpLibTest-msvc71.vcproj

rem Synchronize sipXtackLib projects
java -jar %SYNCHRONIZER_JAR% sipXtackLib\sipXtackLib-msvc8.vcproj sipXtackLib\sipXtackLib-msvc71.vcproj
java -jar %SYNCHRONIZER_JAR% sipXtackLib\sipXtackLibTest-msvc8.vcproj sipXtackLib\sipXtackLibTest-msvc71.vcproj

echo Done.
