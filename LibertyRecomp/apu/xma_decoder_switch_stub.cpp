#include <stdafx.h>
#include <kernel/function.h>

struct XMAPLAYBACKINIT {
    be<uint32_t> sampleRate;
    be<uint32_t> outputBufferSize;
    uint8_t channelCount;
    uint8_t subframes;
};

struct XMAPLAYBACKLOOP {
    be<uint32_t> loopStartOffset;
    be<uint32_t> loopEndOffset;
    uint8_t loopSubframeEnd;
    uint8_t loopSubframeSkip;
};

struct XmaPlaybackStub {
    uint32_t streamPosition = 0;
    uint32_t pendingBytes = 0;
    bool locked = false;
};

static XmaPlaybackStub g_xmaPlayback;

uint32_t XMAPlaybackCreate(uint32_t streams, XMAPLAYBACKINIT* init, uint32_t flags, be<uint32_t>* outPlayback) {
    (void)streams;
    (void)init;
    (void)flags;
    g_xmaPlayback = {};
    if (outPlayback != nullptr) {
        *outPlayback = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(&g_xmaPlayback));
    }
    return 0;
}

uint32_t XMAPlaybackRequestModifyLock(XmaPlaybackStub* playback) {
    if (playback != nullptr) {
        playback->locked = true;
    }
    return 0;
}

uint32_t XMAPlaybackWaitUntilModifyLockObtained(XmaPlaybackStub* playback) {
    if (playback != nullptr) {
        playback->locked = true;
    }
    return 0;
}

uint32_t XMAPlaybackQueryReadyForMoreData(XmaPlaybackStub* playback, uint32_t stream) {
    (void)playback;
    (void)stream;
    return 1;
}

uint32_t XMAPlaybackIsIdle(XmaPlaybackStub* playback, uint32_t stream) {
    (void)playback;
    (void)stream;
    return 1;
}

uint32_t XMAPlaybackQueryContextsAllocated(XmaPlaybackStub* playback) {
    (void)playback;
    return 0;
}

uint32_t XMAPlaybackResumePlayback(XmaPlaybackStub* playback) {
    (void)playback;
    return 0;
}

uint32_t XMAPlaybackQueryInputDataPending(XmaPlaybackStub* playback, uint32_t stream, uint32_t data) {
    (void)stream;
    (void)data;
    return playback != nullptr ? playback->pendingBytes : 0;
}

uint32_t XMAPlaybackGetErrorBits(XmaPlaybackStub* playback, uint32_t stream) {
    (void)playback;
    (void)stream;
    return 0;
}

uint32_t XMAPlaybackSubmitData(XmaPlaybackStub* playback, uint32_t stream, uint32_t data, uint32_t dataSize) {
    (void)stream;
    (void)data;
    if (playback != nullptr) {
        playback->pendingBytes = dataSize;
    }
    return 0;
}

uint32_t XMAPlaybackQueryAvailableData(XmaPlaybackStub* playback, uint32_t stream) {
    (void)playback;
    (void)stream;
    return 0;
}

uint32_t XMAPlaybackAccessDecodedData(XmaPlaybackStub* playback, uint32_t stream, uint32_t** data) {
    (void)playback;
    (void)stream;
    if (data != nullptr) {
        *data = nullptr;
    }
    return 0;
}

uint32_t XMAPlaybackConsumeDecodedData(XmaPlaybackStub* playback, uint32_t stream, uint32_t maxSamples, uint32_t** data) {
    (void)stream;
    (void)maxSamples;
    if (playback != nullptr) {
        playback->pendingBytes = 0;
    }
    if (data != nullptr) {
        *data = nullptr;
    }
    return 0;
}

uint32_t XMAPlaybackQueryModifyLockObtained(XmaPlaybackStub* playback) {
    return playback != nullptr && playback->locked ? 1 : 0;
}

uint32_t XMAPlaybackDestroy(XmaPlaybackStub* playback) {
    if (playback != nullptr) {
        *playback = {};
    }
    return 0;
}

uint32_t XMAPlaybackFlushData(XmaPlaybackStub* playback) {
    if (playback != nullptr) {
        playback->pendingBytes = 0;
    }
    return 0;
}

uint32_t XmaPlaybackSetLoop(XmaPlaybackStub* playback, uint32_t streamIndex, XMAPLAYBACKLOOP* loop) {
    (void)playback;
    (void)streamIndex;
    (void)loop;
    return 0;
}

uint32_t XMAPlaybackGetRemainingLoopCount(XmaPlaybackStub* playback) {
    (void)playback;
    return 0;
}

uint32_t XMAPlaybackGetStreamPosition(XmaPlaybackStub* playback) {
    return playback != nullptr ? playback->streamPosition : 0;
}

uint32_t XMAPlaybackSetDecodePosition(XmaPlaybackStub* playback, uint32_t streamIndex, uint32_t bitOffset, uint32_t subframeOffset) {
    (void)streamIndex;
    (void)subframeOffset;
    if (playback != nullptr) {
        playback->streamPosition = bitOffset;
    }
    return 0;
}

uint32_t XMAPlaybackRewindDecodePosition(XmaPlaybackStub* playback, uint32_t streamIndex, uint32_t numSamples) {
    (void)streamIndex;
    (void)numSamples;
    if (playback != nullptr) {
        playback->streamPosition = 0;
    }
    return 0;
}

uint32_t XMAPlaybackQueryCurrentPosition(XmaPlaybackStub* playback) {
    return playback != nullptr ? playback->streamPosition : 0;
}

GUEST_FUNCTION_HOOK(sub_8255C090, XMAPlaybackCreate);
GUEST_FUNCTION_HOOK(sub_8255CC48, XMAPlaybackRequestModifyLock);
GUEST_FUNCTION_HOOK(sub_8255CCC8, XMAPlaybackWaitUntilModifyLockObtained);
GUEST_FUNCTION_HOOK(sub_8255C4D0, XMAPlaybackQueryReadyForMoreData);
GUEST_FUNCTION_HOOK(sub_8255C520, XMAPlaybackIsIdle);
GUEST_FUNCTION_HOOK(sub_8255C388, XMAPlaybackQueryContextsAllocated);
GUEST_FUNCTION_HOOK(sub_8255CF10, XMAPlaybackResumePlayback);
GUEST_FUNCTION_HOOK(sub_8255C470, XMAPlaybackQueryInputDataPending);
GUEST_FUNCTION_HOOK(sub_8255C9A0, XMAPlaybackGetErrorBits);
GUEST_FUNCTION_HOOK(sub_8255C398, XMAPlaybackSubmitData);
GUEST_FUNCTION_HOOK(sub_8255C578, XMAPlaybackQueryAvailableData);
GUEST_FUNCTION_HOOK(sub_8255C7A8, XMAPlaybackAccessDecodedData);
GUEST_FUNCTION_HOOK(sub_8255C5F0, XMAPlaybackConsumeDecodedData);
GUEST_FUNCTION_HOOK(sub_8255CD90, XMAPlaybackQueryModifyLockObtained);
GUEST_FUNCTION_HOOK(sub_8255C8D8, XMAPlaybackFlushData);
GUEST_FUNCTION_HOOK(sub_8255C9D8, XmaPlaybackSetLoop);
GUEST_FUNCTION_HOOK(sub_8255CA50, XMAPlaybackGetRemainingLoopCount);
GUEST_FUNCTION_HOOK(sub_8255CA90, XMAPlaybackGetStreamPosition);
GUEST_FUNCTION_HOOK(sub_8255CB20, XMAPlaybackSetDecodePosition);
GUEST_FUNCTION_HOOK(sub_8255C850, XMAPlaybackRewindDecodePosition);
GUEST_FUNCTION_HOOK(sub_8255CAB0, XMAPlaybackQueryCurrentPosition);
GUEST_FUNCTION_HOOK(sub_8255C2C0, XMAPlaybackDestroy);
