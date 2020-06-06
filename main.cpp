#include <iostream>
#include "al.h"
#include "alc.h"

#include <list>

using std::list;

using namespace std;

#define FREQ 22050   // Sample rate
#define CAP_SIZE 512 // How much to capture at a time (affects latency)

int main()
{
    list<ALuint> bufferQueue;
    ALenum errorCode=0;
    ALuint helloBuffer[16], helloSource[1];
    if (alcIsExtensionPresent(NULL, "ALC_ENUMERATE_ALL_EXT") == AL_TRUE){
        const ALCchar *devices;
        const ALCchar *defaultDeviceName;

        devices = alcGetString(NULL, ALC_DEVICE_SPECIFIER);

        cout << devices << endl;

        auto Device = alcOpenDevice(NULL);

        auto Context=alcCreateContext(Device,NULL);
        alcMakeContextCurrent(Context);


        auto errorCode = alcGetError(Device);
        auto* inputDevice = alcCaptureOpenDevice(NULL,FREQ,AL_FORMAT_MONO16,FREQ/2);

        errorCode = alcGetError(inputDevice);
        alcCaptureStart(inputDevice); // Begin capturing
        errorCode = alcGetError(inputDevice);

        alGenBuffers(16,&helloBuffer[0]); // Create some buffer-objects
        errorCode = alGetError();

        // Queue our buffers onto an STL list
        for (int ii=0;ii<16;++ii) {
            bufferQueue.push_back(helloBuffer[ii]);
        }

        alGenSources (1, &helloSource[0]); // Create a sound source
        alSourcef(helloSource[0],AL_SOURCE_TYPE,AL_STREAMING);
        errorCode = alGetError();

        short buffer[FREQ*2];       // A buffer to hold captured audio
        auto samplesIn=0;           // How many samples are captured
        auto availBuffers=0;        // Buffers to be recovered
        auto myBuff;                // The buffer we're using
        auto buffHolder[16];        // An array to hold catch the unqueued buffers
        bool done = false;
        while (!done) {             // Main loop
            // Poll for recoverable buffers
            alGetSourcei(helloSource[0],AL_BUFFERS_PROCESSED,&availBuffers);
            if (availBuffers>0) {
                alSourceUnqueueBuffers(helloSource[0],availBuffers,buffHolder);
                for (int ii=0;ii<availBuffers;++ii) {
                    // Push the recovered buffers back on the queue
                    bufferQueue.push_back(buffHolder[ii]);
                }
            }
            // Poll for captured audio
            alcGetIntegerv(inputDevice,ALC_CAPTURE_SAMPLES,1,&samplesIn);
            if (samplesIn>CAP_SIZE) {
                // Grab the sound
                alcCaptureSamples(inputDevice,buffer,CAP_SIZE);

                //***** Process/filter captured data here *****//
                //for (int ii=0;ii<CAP_SIZE;++ii) {
                  //buffer[ii]*=0.1; // Make it quieter
                  //cout << buffer[ii] << endl;
                //}

                // Stuff the captured data in a buffer-object
                if (!bufferQueue.empty()) { // We just drop the data if no buffers are available
                    myBuff = bufferQueue.front(); bufferQueue.pop_front();
                    alBufferData(myBuff,AL_FORMAT_MONO16,buffer,CAP_SIZE*sizeof(short),FREQ);

                    // Queue the buffer
                    alSourceQueueBuffers(helloSource[0],1,&myBuff);

                    // Restart the source if needed
                    // (if we take too long and the queue dries up,
                    //  the source stops playing).
                    ALint sState=0;
                    alGetSourcei(helloSource[0],AL_SOURCE_STATE,&sState);
                    //alSourcef(helloSource[0],AL_PITCH,1.2f);
                    if (sState!=AL_PLAYING) {
                        alSourcePlay(helloSource[0]);
                    }
                }
            }
        }
        // Stop capture
        alcCaptureStop(inputDevice);
        alcCaptureCloseDevice(inputDevice);

        // Stop the sources
        alSourceStopv(1,&helloSource[0]);
        for (int ii=0;ii<1;++ii) {
            alSourcei(helloSource[ii],AL_BUFFER,0);
        }
        // Clean-up
        alDeleteSources(1,&helloSource[0]);
        alDeleteBuffers(16,&helloBuffer[0]);
        errorCode = alGetError();
        alcMakeContextCurrent(NULL);
        errorCode = alGetError();
        alcDestroyContext(Context);
        alcCloseDevice(Device);
    }
    return 0;
}
