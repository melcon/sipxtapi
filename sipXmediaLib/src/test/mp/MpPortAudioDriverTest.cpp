//  
// Copyright (C) 2007 Jaroslav Libak
//
// $$
///////////////////////////////////////////////////////////////////////////////

#include <cppunit/extensions/HelperMacros.h>
#include <cppunit/TestCase.h>
#include <sipxunit/TestUtilities.h>
#include <os/OsStatus.h>
#include <os/OsTask.h>
#include <mp/MpAudioDriverFactory.h>
#include <mp/MpAudioDriverBase.h>
#include <mp/MpAudioDriverDefs.h>
#include <mp/MpAudioDeviceInfo.h>
#include <mp/MpAudioStreamInfo.h>
#include <mp/MpAudioStreamParameters.h>

class MpPortAudioDriverTest : public CppUnit::TestCase
{
   CPPUNIT_TEST_SUITE(MpPortAudioDriverTest);
   CPPUNIT_TEST(getDriverName);
   CPPUNIT_TEST(getDriverVersion);
   CPPUNIT_TEST(getHostApiCount);
   CPPUNIT_TEST(getDefaultHostApi);
   CPPUNIT_TEST(getHostApiInfo);
   CPPUNIT_TEST(hostApiTypeIdToHostApiIndex);
   CPPUNIT_TEST(hostApiDeviceIndexToDeviceIndex);
   CPPUNIT_TEST(getDeviceCount);
   CPPUNIT_TEST(getDefaultInputDevice);
   CPPUNIT_TEST(getDefaultOutputDevice);
   CPPUNIT_TEST(getDeviceInfo);
   CPPUNIT_TEST(getSampleSize);

   CPPUNIT_TEST(openDefaultStream);
   CPPUNIT_TEST(closeStream);
   CPPUNIT_TEST(startStream);
   CPPUNIT_TEST(stopStream);
   CPPUNIT_TEST(abortStream);
   CPPUNIT_TEST(getStreamInfo);
   CPPUNIT_TEST(getStreamTime);
   CPPUNIT_TEST(getStreamCpuLoad);
//   CPPUNIT_TEST(getStreamReadAvailable); // currently fails, portaudio doesnt seem to support this function for all drivers
//   CPPUNIT_TEST(getStreamWriteAvailable); // currently fails, portaudio doesnt seem to support this function for all drivers
   CPPUNIT_TEST(isFormatSupported);
   CPPUNIT_TEST(isStreamStopped);
   CPPUNIT_TEST(isStreamActive);
   CPPUNIT_TEST(openStream);

   CPPUNIT_TEST_SUITE_END();

private:
   MpAudioDriverBase* m_pDriver;

public:

   void setUp()
   {
      m_pDriver = MpAudioDriverFactory::createAudioDriver(MpAudioDriverFactory::AUDIO_DRIVER_PORTAUDIO);
      CPPUNIT_ASSERT(m_pDriver);
   }

   void tearDown()
   {
      m_pDriver->release();
      m_pDriver = NULL;
   }

   void getDriverName()
   {
      UtlString driverName = m_pDriver->getDriverName();
      CPPUNIT_ASSERT(driverName.index("portaudio", 0, UtlString::ignoreCase) != UTL_NOT_FOUND);
      printf("driverName = %s\n", driverName.data());
   }

   void getDriverVersion()
   {
      UtlString driverVersion = m_pDriver->getDriverVersion();
      CPPUNIT_ASSERT(driverVersion.index("V19", 0, UtlString::ignoreCase) != UTL_NOT_FOUND);
      printf("driverVersion = %s\n", driverVersion.data());
   }

   void getHostApiCount()
   {
      int count = 0;
      OsStatus res = m_pDriver->getHostApiCount(count);
      CPPUNIT_ASSERT(res == OS_SUCCESS);
      CPPUNIT_ASSERT(count > 0);
      printf("hostApiCount = %d\n", count);
   }

   void getDefaultHostApi()
   {
      MpHostAudioApiIndex defaultHostApiIndex = 0;
      OsStatus res = m_pDriver->getDefaultHostApi(defaultHostApiIndex);
      CPPUNIT_ASSERT(res == OS_SUCCESS);
      printf("defaultHostApiIndex = %d\n", defaultHostApiIndex);
      CPPUNIT_ASSERT(defaultHostApiIndex >= 0);
   }

   void getHostApiInfo()
   {
      int count = 0;
      m_pDriver->getHostApiCount(count);

      printf("------ Host api list ------\n");
      for (int i = 0; i < count; i++)
      {
         MpHostAudioApiInfo apiInfo;
         OsStatus res = m_pDriver->getHostApiInfo(i, apiInfo);
         CPPUNIT_ASSERT(res == OS_SUCCESS);
         printf("m_typeId = %d\n", (int)apiInfo.getTypeId());
         printf("m_name = %s\n", apiInfo.getName().data());
         printf("m_deviceCount = %d\n", apiInfo.getDeviceCount());
         printf("m_defaultInputDevice = %d\n", apiInfo.getDefaultInputDevice());
         printf("m_defaultOutputDevice = %d\n", apiInfo.getDefaultOutputDevice());
         printf("---------------------------\n");
         CPPUNIT_ASSERT(apiInfo.getDeviceCount() >= 0);
         CPPUNIT_ASSERT(apiInfo.getDefaultInputDevice() >= 0);
         CPPUNIT_ASSERT(apiInfo.getDefaultInputDevice() >= 0);
      }      
   }

   void hostApiTypeIdToHostApiIndex()
   {
      int count = 0;
      m_pDriver->getHostApiCount(count);

      for (int i = 0; i < count; i++)
      {
         MpHostAudioApiInfo apiInfo;
         MpHostAudioApiIndex hostApiIndex = 0;
         OsStatus res = m_pDriver->getHostApiInfo(i, apiInfo);
         CPPUNIT_ASSERT(res == OS_SUCCESS);
         MpHostAudioApiTypeId typeId = apiInfo.getTypeId();
         res = m_pDriver->hostApiTypeIdToHostApiIndex(typeId, hostApiIndex);
         CPPUNIT_ASSERT(res == OS_SUCCESS);
         CPPUNIT_ASSERT(hostApiIndex == i);
      }
   }

   void hostApiDeviceIndexToDeviceIndex()
   {
      int count = 0;
      m_pDriver->getHostApiCount(count);

      for (int i = 0; i < count; i++)
      {
         MpHostAudioApiInfo apiInfo;
         MpHostAudioApiIndex hostApiIndex = 0;
         m_pDriver->getHostApiInfo(i, apiInfo);

         for (int k = 0; k < apiInfo.getDeviceCount(); k++)
         {
            MpAudioDeviceIndex audioDeviceIndex = 0;
            OsStatus res = m_pDriver->hostApiDeviceIndexToDeviceIndex(i, k, audioDeviceIndex);
            CPPUNIT_ASSERT(res == OS_SUCCESS);
         }
      }
   }

   void getDeviceCount()
   {
      int count = 0;
      OsStatus res = m_pDriver->getDeviceCount(count);
      CPPUNIT_ASSERT(res == OS_SUCCESS);
      CPPUNIT_ASSERT(count >= 0);
   }

   void getDefaultInputDevice()
   {
      int count = 0;
      m_pDriver->getDeviceCount(count);
      MpAudioDeviceIndex deviceIndex = 0;
      OsStatus res = m_pDriver->getDefaultInputDevice(deviceIndex);
      CPPUNIT_ASSERT(res == OS_SUCCESS);
      CPPUNIT_ASSERT(deviceIndex >= 0 && deviceIndex < count);
   }

   void getDefaultOutputDevice()
   {
      int count = 0;
      m_pDriver->getDeviceCount(count);
      MpAudioDeviceIndex deviceIndex = 0;
      OsStatus res = m_pDriver->getDefaultOutputDevice(deviceIndex);
      CPPUNIT_ASSERT(res == OS_SUCCESS);
      CPPUNIT_ASSERT(deviceIndex >= 0 && deviceIndex < count);
   }

   void getDeviceInfo()
   {
      int count = 0;
      m_pDriver->getDeviceCount(count);
      printf("------ Device list ------\n");
      for (int i = 0; i < count; i++)
      {
         MpAudioDeviceInfo deviceInfo;
         OsStatus res = m_pDriver->getDeviceInfo(i, deviceInfo);
         CPPUNIT_ASSERT(res == OS_SUCCESS);
         printf("deviceIndex = %d\n", i);
         printf("m_name = %s\n", deviceInfo.getName().data());
         printf("m_hostApi = %d\n", deviceInfo.getHostApi());
         printf("m_maxInputChannels = %d\n", deviceInfo.getMaxInputChannels());
         printf("m_maxOutputChannels = %d\n", deviceInfo.getMaxOutputChannels());
         printf("m_defaultLowInputLatency = %f\n", deviceInfo.getDefaultLowInputLatency());
         printf("m_defaultLowOutputLatency = %f\n", deviceInfo.getDefaultLowOutputLatency());
         printf("m_defaultHighInputLatency = %f\n", deviceInfo.getDefaultHighInputLatency());
         printf("m_defaultHighOutputLatency = %f\n", deviceInfo.getDefaultHighOutputLatency());
         printf("m_defaultSampleRate = %f\n", deviceInfo.getDefaultSampleRate());
         printf("-------------------------\n");
         CPPUNIT_ASSERT(deviceInfo.getHostApi() >= 0);
         CPPUNIT_ASSERT(deviceInfo.getMaxInputChannels() >= 0);
         CPPUNIT_ASSERT(deviceInfo.getMaxOutputChannels() >= 0);
         CPPUNIT_ASSERT(deviceInfo.getDefaultLowInputLatency() >= 0);
         CPPUNIT_ASSERT(deviceInfo.getDefaultLowOutputLatency() >= 0);
         CPPUNIT_ASSERT(deviceInfo.getDefaultHighInputLatency() >= 0);
         CPPUNIT_ASSERT(deviceInfo.getDefaultHighOutputLatency() >= 0);
         CPPUNIT_ASSERT(deviceInfo.getDefaultSampleRate() > 0);
      }
   }

   void getSampleSize()
   {
      int sampleSize = 0;
      OsStatus res = OS_FAILED;
      // test interleaved
      res = m_pDriver->getSampleSize(MP_AUDIO_FORMAT_FLOAT32, sampleSize);
      CPPUNIT_ASSERT(res == OS_SUCCESS);
      CPPUNIT_ASSERT(sampleSize > 0);
      res = m_pDriver->getSampleSize(MP_AUDIO_FORMAT_INT32, sampleSize);
      CPPUNIT_ASSERT(res == OS_SUCCESS);
      CPPUNIT_ASSERT(sampleSize > 0);
      res = m_pDriver->getSampleSize(MP_AUDIO_FORMAT_INT24, sampleSize);
      CPPUNIT_ASSERT(res == OS_SUCCESS);
      CPPUNIT_ASSERT(sampleSize > 0);
      res = m_pDriver->getSampleSize(MP_AUDIO_FORMAT_INT16, sampleSize);
      CPPUNIT_ASSERT(res == OS_SUCCESS);
      CPPUNIT_ASSERT(sampleSize > 0);
      res = m_pDriver->getSampleSize(MP_AUDIO_FORMAT_INT8, sampleSize);
      CPPUNIT_ASSERT(res == OS_SUCCESS);
      CPPUNIT_ASSERT(sampleSize > 0);
      res = m_pDriver->getSampleSize(MP_AUDIO_FORMAT_UINT8, sampleSize);
      CPPUNIT_ASSERT(res == OS_SUCCESS);
      CPPUNIT_ASSERT(sampleSize > 0);

      // test non interleaved
      res = m_pDriver->getSampleSize(MP_AUDIO_FORMAT_FLOAT32 | MP_AUDIO_FORMAT_NONINTERLEAVED, sampleSize);
      CPPUNIT_ASSERT(res == OS_SUCCESS);
      CPPUNIT_ASSERT(sampleSize > 0);
      res = m_pDriver->getSampleSize(MP_AUDIO_FORMAT_INT32 | MP_AUDIO_FORMAT_NONINTERLEAVED, sampleSize);
      CPPUNIT_ASSERT(res == OS_SUCCESS);
      CPPUNIT_ASSERT(sampleSize > 0);
      res = m_pDriver->getSampleSize(MP_AUDIO_FORMAT_INT24 | MP_AUDIO_FORMAT_NONINTERLEAVED, sampleSize);
      CPPUNIT_ASSERT(res == OS_SUCCESS);
      CPPUNIT_ASSERT(sampleSize > 0);
      res = m_pDriver->getSampleSize(MP_AUDIO_FORMAT_INT16 | MP_AUDIO_FORMAT_NONINTERLEAVED, sampleSize);
      CPPUNIT_ASSERT(res == OS_SUCCESS);
      CPPUNIT_ASSERT(sampleSize > 0);
      res = m_pDriver->getSampleSize(MP_AUDIO_FORMAT_INT8 | MP_AUDIO_FORMAT_NONINTERLEAVED, sampleSize);
      CPPUNIT_ASSERT(res == OS_SUCCESS);
      CPPUNIT_ASSERT(sampleSize > 0);
      res = m_pDriver->getSampleSize(MP_AUDIO_FORMAT_UINT8 | MP_AUDIO_FORMAT_NONINTERLEAVED, sampleSize);
      CPPUNIT_ASSERT(res == OS_SUCCESS);
      CPPUNIT_ASSERT(sampleSize > 0);
   }

   void openDefaultStream()
   {
      for (int i = 0; i < 2; i++)
      {
         // first try asynchronous, then synchronous

         for (int channel = 1; channel < 3; channel++)
         {
            OsStatus res = OS_FAILED;
            MpAudioStreamId stream;

            // first try 1 channel, then 2 channel
            res = m_pDriver->openDefaultStream(&stream,
               1,
               channel,
               MP_AUDIO_FORMAT_INT16,
               8000,
               160,
               i);
            CPPUNIT_ASSERT(res == OS_SUCCESS);
            OsTask::delay(50);
            res = m_pDriver->closeStream(stream);
            CPPUNIT_ASSERT(res == OS_SUCCESS);

            res = m_pDriver->openDefaultStream(&stream,
               1,
               channel,
               MP_AUDIO_FORMAT_FLOAT32,
               8000,
               160,
               i);
            CPPUNIT_ASSERT(res == OS_SUCCESS);
            OsTask::delay(50);
            res = m_pDriver->closeStream(stream);
            CPPUNIT_ASSERT(res == OS_SUCCESS);

            res = m_pDriver->openDefaultStream(&stream,
               1,
               channel,
               MP_AUDIO_FORMAT_INT32,
               8000,
               160,
               i);
            CPPUNIT_ASSERT(res == OS_SUCCESS);
            OsTask::delay(50);
            res = m_pDriver->closeStream(stream);
            CPPUNIT_ASSERT(res == OS_SUCCESS);

            res = m_pDriver->openDefaultStream(&stream,
               1,
               channel,
               MP_AUDIO_FORMAT_INT24,
               8000,
               160,
               i);
            CPPUNIT_ASSERT(res == OS_SUCCESS);
            OsTask::delay(50);
            res = m_pDriver->closeStream(stream);
            CPPUNIT_ASSERT(res == OS_SUCCESS);

            res = m_pDriver->openDefaultStream(&stream,
               1,
               channel,
               MP_AUDIO_FORMAT_INT8,
               8000,
               160,
               i);
            CPPUNIT_ASSERT(res == OS_SUCCESS);
            OsTask::delay(50);
            res = m_pDriver->closeStream(stream);
            CPPUNIT_ASSERT(res == OS_SUCCESS);

            res = m_pDriver->openDefaultStream(&stream,
               1,
               channel,
               MP_AUDIO_FORMAT_UINT8,
               8000,
               160,
               i);
            CPPUNIT_ASSERT(res == OS_SUCCESS);
            OsTask::delay(50);
            res = m_pDriver->closeStream(stream);
            CPPUNIT_ASSERT(res == OS_SUCCESS);

            // try non interleaved
            res = m_pDriver->openDefaultStream(&stream,
               1,
               channel,
               MP_AUDIO_FORMAT_UINT8 | MP_AUDIO_FORMAT_NONINTERLEAVED,
               8000,
               160,
               i);
            CPPUNIT_ASSERT(res == OS_SUCCESS);
            OsTask::delay(50);
            res = m_pDriver->closeStream(stream);
            CPPUNIT_ASSERT(res == OS_SUCCESS);

            res = m_pDriver->openDefaultStream(&stream,
               1,
               channel,
               MP_AUDIO_FORMAT_INT8 | MP_AUDIO_FORMAT_NONINTERLEAVED,
               8000,
               160,
               i);
            CPPUNIT_ASSERT(res == OS_SUCCESS);
            OsTask::delay(50);
            res = m_pDriver->closeStream(stream);
            CPPUNIT_ASSERT(res == OS_SUCCESS);

            res = m_pDriver->openDefaultStream(&stream,
               1,
               channel,
               MP_AUDIO_FORMAT_INT16 | MP_AUDIO_FORMAT_NONINTERLEAVED,
               8000,
               160,
               i);
            CPPUNIT_ASSERT(res == OS_SUCCESS);
            OsTask::delay(50);
            res = m_pDriver->closeStream(stream);
            CPPUNIT_ASSERT(res == OS_SUCCESS);

            res = m_pDriver->openDefaultStream(&stream,
               1,
               channel,
               MP_AUDIO_FORMAT_INT24 | MP_AUDIO_FORMAT_NONINTERLEAVED,
               8000,
               160,
               i);
            CPPUNIT_ASSERT(res == OS_SUCCESS);
            OsTask::delay(50);
            res = m_pDriver->closeStream(stream);
            CPPUNIT_ASSERT(res == OS_SUCCESS);

            res = m_pDriver->openDefaultStream(&stream,
               1,
               channel,
               MP_AUDIO_FORMAT_INT32 | MP_AUDIO_FORMAT_NONINTERLEAVED,
               8000,
               160,
               i);
            CPPUNIT_ASSERT(res == OS_SUCCESS);
            OsTask::delay(50);
            res = m_pDriver->closeStream(stream);
            CPPUNIT_ASSERT(res == OS_SUCCESS);

            res = m_pDriver->openDefaultStream(&stream,
               1,
               channel,
               MP_AUDIO_FORMAT_FLOAT32 | MP_AUDIO_FORMAT_NONINTERLEAVED,
               8000,
               160,
               i);
            CPPUNIT_ASSERT(res == OS_SUCCESS);
            OsTask::delay(50);
            res = m_pDriver->closeStream(stream);
            CPPUNIT_ASSERT(res == OS_SUCCESS);
         }         
      }
   }

   void closeStream()
   {
      for (int sync = 0; sync < 2; sync++)
      {
         OsStatus res = OS_FAILED;
         MpAudioStreamId stream = 0;

         res = m_pDriver->closeStream(stream);
         CPPUNIT_ASSERT(res == OS_FAILED);

         res = m_pDriver->openDefaultStream(&stream,
            1,
            1,
            MP_AUDIO_FORMAT_INT16,
            8000,
            160,
            sync);
         CPPUNIT_ASSERT(res == OS_SUCCESS);
         OsTask::delay(50);
         res = m_pDriver->closeStream(stream);
         CPPUNIT_ASSERT(res == OS_SUCCESS);
         res = m_pDriver->closeStream(stream);
         CPPUNIT_ASSERT(res == OS_FAILED);
      }      
   }

   void startStream()
   {
      for (int sync = 0; sync < 2; sync++)
      {
         OsStatus res = OS_FAILED;
         MpAudioStreamId stream = 0;

         res = m_pDriver->startStream(stream);
         CPPUNIT_ASSERT(res == OS_FAILED);

         res = m_pDriver->openDefaultStream(&stream,
            1,
            1,
            MP_AUDIO_FORMAT_INT16,
            8000,
            160,
            sync);
         CPPUNIT_ASSERT(res == OS_SUCCESS);
         OsTask::delay(50);
         res = m_pDriver->startStream(stream);
         CPPUNIT_ASSERT(res == OS_SUCCESS);
         OsTask::delay(50);
         res = m_pDriver->stopStream(stream);
         CPPUNIT_ASSERT(res == OS_SUCCESS);
         OsTask::delay(50);
         res = m_pDriver->startStream(stream);
         CPPUNIT_ASSERT(res == OS_SUCCESS);
         res = m_pDriver->stopStream(stream);
         CPPUNIT_ASSERT(res == OS_SUCCESS);
         res = m_pDriver->stopStream(stream);
         CPPUNIT_ASSERT(res == OS_FAILED);
         res = m_pDriver->closeStream(stream);
         CPPUNIT_ASSERT(res == OS_SUCCESS);
      }
   }

   void stopStream()
   {
      for (int sync = 0; sync < 2; sync++)
      {
         OsStatus res = OS_FAILED;
         MpAudioStreamId stream = 0;

         res = m_pDriver->stopStream(stream);
         CPPUNIT_ASSERT(res == OS_FAILED);

         res = m_pDriver->openDefaultStream(&stream,
            1,
            1,
            MP_AUDIO_FORMAT_INT16,
            8000,
            160,
            sync);
         CPPUNIT_ASSERT(res == OS_SUCCESS);
         OsTask::delay(50);
         res = m_pDriver->startStream(stream);
         CPPUNIT_ASSERT(res == OS_SUCCESS);
         OsTask::delay(50);
         res = m_pDriver->stopStream(stream);
         CPPUNIT_ASSERT(res == OS_SUCCESS);
         res = m_pDriver->stopStream(stream);
         CPPUNIT_ASSERT(res == OS_FAILED);
         res = m_pDriver->closeStream(stream);
         CPPUNIT_ASSERT(res == OS_SUCCESS);
      }
   }

   void abortStream()
   {
      for (int sync = 0; sync < 2; sync++)
      {
         OsStatus res = OS_FAILED;
         MpAudioStreamId stream = 0;

         res = m_pDriver->abortStream(stream);
         CPPUNIT_ASSERT(res == OS_FAILED);

         res = m_pDriver->openDefaultStream(&stream,
            1,
            1,
            MP_AUDIO_FORMAT_INT16,
            8000,
            160,
            sync);
         CPPUNIT_ASSERT(res == OS_SUCCESS);
         OsTask::delay(50);
         res = m_pDriver->startStream(stream);
         CPPUNIT_ASSERT(res == OS_SUCCESS);
         OsTask::delay(50);
         res = m_pDriver->abortStream(stream);
         CPPUNIT_ASSERT(res == OS_SUCCESS);
         res = m_pDriver->abortStream(stream);
         CPPUNIT_ASSERT(res == OS_FAILED);
         res = m_pDriver->closeStream(stream);
         CPPUNIT_ASSERT(res == OS_SUCCESS);
      }
   }

   void getStreamInfo()
   {
      OsStatus res = OS_FAILED;
      MpAudioStreamId stream = 0;

      res = m_pDriver->openDefaultStream(&stream,
         1,
         1,
         MP_AUDIO_FORMAT_INT16,
         8000,
         160,
         TRUE);
      CPPUNIT_ASSERT(res == OS_SUCCESS);
      res = m_pDriver->startStream(stream);
      CPPUNIT_ASSERT(res == OS_SUCCESS);
      OsTask::delay(50);

      MpAudioStreamInfo streamInfo;
      res = m_pDriver->getStreamInfo(stream, streamInfo);
      CPPUNIT_ASSERT(res == OS_SUCCESS);
      CPPUNIT_ASSERT(streamInfo.getInputLatency() >= 0);
      CPPUNIT_ASSERT(streamInfo.getOutputLatency() >= 0);

      res = m_pDriver->stopStream(stream);
      CPPUNIT_ASSERT(res == OS_SUCCESS);
      res = m_pDriver->closeStream(stream);
      CPPUNIT_ASSERT(res == OS_SUCCESS);

      res = m_pDriver->getStreamInfo(NULL, streamInfo);
      CPPUNIT_ASSERT(res == OS_FAILED);
   }

   void getStreamTime()
   {
      OsStatus res = OS_FAILED;
      MpAudioStreamId stream = 0;

      res = m_pDriver->openDefaultStream(&stream,
         1,
         1,
         MP_AUDIO_FORMAT_INT16,
         8000,
         160,
         TRUE);
      CPPUNIT_ASSERT(res == OS_SUCCESS);
      res = m_pDriver->startStream(stream);
      CPPUNIT_ASSERT(res == OS_SUCCESS);
      OsTask::delay(50);

      double streamTime;
      res = m_pDriver->getStreamTime(stream, streamTime);
      CPPUNIT_ASSERT(res == OS_SUCCESS);

      res = m_pDriver->stopStream(stream);
      CPPUNIT_ASSERT(res == OS_SUCCESS);
      res = m_pDriver->closeStream(stream);
      CPPUNIT_ASSERT(res == OS_SUCCESS);

      res = m_pDriver->getStreamTime(NULL, streamTime);
      CPPUNIT_ASSERT(res == OS_FAILED);
   }

   void getStreamCpuLoad()
   {
      OsStatus res = OS_FAILED;
      MpAudioStreamId stream = 0;

      res = m_pDriver->openDefaultStream(&stream,
         1,
         1,
         MP_AUDIO_FORMAT_INT16,
         8000,
         160,
         TRUE);
      CPPUNIT_ASSERT(res == OS_SUCCESS);
      res = m_pDriver->startStream(stream);
      CPPUNIT_ASSERT(res == OS_SUCCESS);
      OsTask::delay(50);

      double cpuLoad;
      res = m_pDriver->getStreamCpuLoad(stream, cpuLoad);
      CPPUNIT_ASSERT(res == OS_SUCCESS);

      res = m_pDriver->stopStream(stream);
      CPPUNIT_ASSERT(res == OS_SUCCESS);
      res = m_pDriver->closeStream(stream);
      CPPUNIT_ASSERT(res == OS_SUCCESS);

      res = m_pDriver->getStreamTime(NULL, cpuLoad);
      CPPUNIT_ASSERT(res == OS_FAILED);
   }

   void getStreamReadAvailable()
   {
      OsStatus res = OS_FAILED;
      MpAudioStreamId stream = 0;

      res = m_pDriver->openDefaultStream(&stream,
         1,
         1,
         MP_AUDIO_FORMAT_INT16,
         8000,
         160,
         TRUE);
      CPPUNIT_ASSERT(res == OS_SUCCESS);
      res = m_pDriver->startStream(stream);
      CPPUNIT_ASSERT(res == OS_SUCCESS);
      OsTask::delay(100);

      // now it should be possible to read some data without blocking
      long framesAvailable = 0;
      res = m_pDriver->getStreamReadAvailable(stream, framesAvailable);
      CPPUNIT_ASSERT(res == OS_SUCCESS);
      CPPUNIT_ASSERT(framesAvailable > 0);

      res = m_pDriver->stopStream(stream);
      CPPUNIT_ASSERT(res == OS_SUCCESS);
      res = m_pDriver->closeStream(stream);
      CPPUNIT_ASSERT(res == OS_SUCCESS);

      res = m_pDriver->getStreamReadAvailable(NULL, framesAvailable);
      CPPUNIT_ASSERT(res == OS_FAILED);
   }

   void getStreamWriteAvailable()
   {
      OsStatus res = OS_FAILED;
      MpAudioStreamId stream = 0;

      res = m_pDriver->openDefaultStream(&stream,
         1,
         1,
         MP_AUDIO_FORMAT_INT16,
         8000,
         160,
         TRUE);
      CPPUNIT_ASSERT(res == OS_SUCCESS);
      res = m_pDriver->startStream(stream);
      CPPUNIT_ASSERT(res == OS_SUCCESS);
      OsTask::delay(100);

      // now it should be possible to read some data without blocking
      long framesAvailable = 0;
      res = m_pDriver->getStreamWriteAvailable(stream, framesAvailable);
      CPPUNIT_ASSERT(res == OS_SUCCESS);
      CPPUNIT_ASSERT(framesAvailable > 0);

      res = m_pDriver->stopStream(stream);
      CPPUNIT_ASSERT(res == OS_SUCCESS);
      res = m_pDriver->closeStream(stream);
      CPPUNIT_ASSERT(res == OS_SUCCESS);

      res = m_pDriver->getStreamReadAvailable(NULL, framesAvailable);
      CPPUNIT_ASSERT(res == OS_FAILED);
   }

   void isFormatSupported()
   {
      OsStatus res = OS_FAILED;
      MpAudioStreamParameters inputParameters;
      MpAudioStreamParameters outputParameters;
      MpAudioDeviceIndex inputDeviceIndex = 0;
      MpAudioDeviceIndex outputDeviceIndex = 0;

      inputParameters.setChannelCount(1);
      outputParameters.setChannelCount(1);
      inputParameters.setSampleFormat(MP_AUDIO_FORMAT_INT16);
      outputParameters.setSampleFormat(MP_AUDIO_FORMAT_INT16);
      inputParameters.setSuggestedLatency(0.1);
      outputParameters.setSuggestedLatency(0.1);

      res = m_pDriver->getDefaultInputDevice(inputDeviceIndex);
      CPPUNIT_ASSERT(res == OS_SUCCESS);
      res = m_pDriver->getDefaultOutputDevice(outputDeviceIndex);
      CPPUNIT_ASSERT(res == OS_SUCCESS);

      inputParameters.setDeviceIndex(inputDeviceIndex);
      outputParameters.setDeviceIndex(outputDeviceIndex);

      res = m_pDriver->isFormatSupported(&inputParameters, &outputParameters, 8000);
      CPPUNIT_ASSERT(res == OS_SUCCESS);
      res = m_pDriver->isFormatSupported(&inputParameters, &outputParameters, 44100);
      CPPUNIT_ASSERT(res == OS_SUCCESS);
   }

   void isStreamStopped()
   {
      OsStatus res = OS_FAILED;
      MpAudioStreamId stream = 0;

      res = m_pDriver->openDefaultStream(&stream,
         1,
         1,
         MP_AUDIO_FORMAT_INT16,
         8000,
         160,
         TRUE);
      CPPUNIT_ASSERT(res == OS_SUCCESS);
      res = m_pDriver->startStream(stream);
      CPPUNIT_ASSERT(res == OS_SUCCESS);
      OsTask::delay(100);

      UtlBoolean isStopped = FALSE;
      res = m_pDriver->isStreamStopped(stream, isStopped);
      CPPUNIT_ASSERT(res == OS_SUCCESS);
      CPPUNIT_ASSERT(isStopped == FALSE);

      res = m_pDriver->stopStream(stream);
      CPPUNIT_ASSERT(res == OS_SUCCESS);

      res = m_pDriver->isStreamStopped(stream, isStopped);
      CPPUNIT_ASSERT(res == OS_SUCCESS);
      CPPUNIT_ASSERT(isStopped == TRUE);

      res = m_pDriver->closeStream(stream);
      CPPUNIT_ASSERT(res == OS_SUCCESS);

      res = m_pDriver->isStreamStopped(NULL, isStopped);
      CPPUNIT_ASSERT(res == OS_FAILED);
   }

   void isStreamActive()
   {
      OsStatus res = OS_FAILED;
      MpAudioStreamId stream = 0;

      res = m_pDriver->openDefaultStream(&stream,
         1,
         1,
         MP_AUDIO_FORMAT_INT16,
         8000,
         160,
         TRUE);
      CPPUNIT_ASSERT(res == OS_SUCCESS);
      res = m_pDriver->startStream(stream);
      CPPUNIT_ASSERT(res == OS_SUCCESS);
      OsTask::delay(100);

      UtlBoolean isActive = FALSE;
      res = m_pDriver->isStreamActive(stream, isActive);
      CPPUNIT_ASSERT(res == OS_SUCCESS);
      CPPUNIT_ASSERT(isActive == TRUE);

      res = m_pDriver->stopStream(stream);
      CPPUNIT_ASSERT(res == OS_SUCCESS);

      res = m_pDriver->isStreamActive(stream, isActive);
      CPPUNIT_ASSERT(res == OS_SUCCESS);
      CPPUNIT_ASSERT(isActive == FALSE);

      res = m_pDriver->closeStream(stream);
      CPPUNIT_ASSERT(res == OS_SUCCESS);

      res = m_pDriver->isStreamActive(NULL, isActive);
      CPPUNIT_ASSERT(res == OS_FAILED);
   }

   void openStream()
   {
      for (int frames = 0; frames < 2; frames++)
      {
         // try fixed number of frames per buffer, and MP_AUDIO_STREAM_FRAMESPERBUFFERUNSPECIFIED
         for (int i = 0; i < 2; i++)
         {
            // first try asynchronous, then synchronous
            for (int channel = 1; channel < 3; channel++)
            {
               OsStatus res = OS_FAILED;
               MpAudioStreamId stream = 0;
               MpAudioStreamParameters inputParameters;
               MpAudioStreamParameters outputParameters;
               MpAudioDeviceIndex inputDeviceIndex = 0;
               MpAudioDeviceIndex outputDeviceIndex = 0;

               inputParameters.setChannelCount(1);
               outputParameters.setChannelCount(channel);
               inputParameters.setSampleFormat(MP_AUDIO_FORMAT_INT16);
               outputParameters.setSampleFormat(MP_AUDIO_FORMAT_INT16);
               inputParameters.setSuggestedLatency(0.1);
               outputParameters.setSuggestedLatency(0.1);

               res = m_pDriver->getDefaultInputDevice(inputDeviceIndex);
               CPPUNIT_ASSERT(res == OS_SUCCESS);
               res = m_pDriver->getDefaultOutputDevice(outputDeviceIndex);
               CPPUNIT_ASSERT(res == OS_SUCCESS);

               inputParameters.setDeviceIndex(inputDeviceIndex);
               outputParameters.setDeviceIndex(outputDeviceIndex);

               res = m_pDriver->openStream(&stream,
                  &inputParameters,
                  &outputParameters,
                  8000,
                  frames == 0 ? MP_AUDIO_STREAM_FRAMESPERBUFFERUNSPECIFIED : 160,
                  MP_AUDIO_STREAM_NOFLAG,
                  i);
               CPPUNIT_ASSERT(res == OS_SUCCESS);

               res = m_pDriver->startStream(stream);
               CPPUNIT_ASSERT(res == OS_SUCCESS);
               OsTask::delay(100);

               res = m_pDriver->stopStream(stream);
               CPPUNIT_ASSERT(res == OS_SUCCESS);

               res = m_pDriver->closeStream(stream);
               CPPUNIT_ASSERT(res == OS_SUCCESS);
            }
         }
      }
   }

};

CPPUNIT_TEST_SUITE_REGISTRATION(MpPortAudioDriverTest);
