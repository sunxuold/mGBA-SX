/* Copyright (c) 2013-2015 Jeffrey Pfau
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#include <mgba/core/sync.h>

#include <mgba/core/blip_buf.h>

static void _changeVideoSync(struct mCoreSync* sync, bool frameOn) {
	// Make sure the video thread can process events while the GBA thread is paused
	MutexLock(&sync->videoFrameMutex);
	if (frameOn != sync->videoFrameOn) {
		sync->videoFrameOn = frameOn;
		ConditionWake(&sync->videoFrameAvailableCond);
	}
	MutexUnlock(&sync->videoFrameMutex);
}

//sunxu added
extern int frameWritedelay;
extern int frameReaddelay;


void mCoreSyncPostFrame(struct mCoreSync* sync) {
	if (!sync) {
		return;
	}
    // thread remove  
	//printf("mCoreSyncPostFrame\n");
	return;
	
	
	
	MutexLock(&sync->videoFrameMutex);
	++sync->videoFramePending;
	/*
	if(sync->videoFrameWait) 
	{
		if(sync->videoFramePending>0)
		{
			++sync->videoFramePending;
			usleep(frameWritedelay);
		}
		else
		{
			++sync->videoFramePending;
		}
	}
	else
	{
		++sync->videoFramePending;
	}
	*/

	do {

		ConditionWake(&sync->videoFrameAvailableCond);
		if (sync->videoFrameWait) {
	
			ConditionWait(&sync->videoFrameRequiredCond, &sync->videoFrameMutex);
			//if(sync->videoFramePending>0) usleep(frameWritedelay);
		}
	} while (sync->videoFrameWait && sync->videoFramePending);
	MutexUnlock(&sync->videoFrameMutex);
}

void mCoreSyncForceFrame(struct mCoreSync* sync) {
	if (!sync) {
		return;
	}

	MutexLock(&sync->videoFrameMutex);
	ConditionWake(&sync->videoFrameAvailableCond);
	MutexUnlock(&sync->videoFrameMutex);
}

bool mCoreSyncWaitFrameStart(struct mCoreSync* sync) {
	if (!sync) {
		return true;
	}
	
	// thread remove
	//printf("mCoreSyncWaitFrameStart\n");
	
	
	MutexLock(&sync->videoFrameMutex);
	ConditionWake(&sync->videoFrameRequiredCond);		

	if (!sync->videoFrameOn && !sync->videoFramePending) {
		
		return false;
	}
	if (sync->videoFrameOn) {		
		if (ConditionWaitTimed(&sync->videoFrameAvailableCond, &sync->videoFrameMutex, 50)) {
			return false;
		}
		//if(sync->videoFramePending ==0)
		//{
		//	usleep(frameReaddelay);
		//	return false;
		//}
	}		
	sync->videoFramePending = 0;

	return true;
}

void mCoreSyncWaitFrameEnd(struct mCoreSync* sync) {
	if (!sync) {
		return;
	}
	
	MutexUnlock(&sync->videoFrameMutex);
}

void mCoreSyncSetVideoSync(struct mCoreSync* sync, bool wait) {
	if (!sync) {
		return;
	}

	_changeVideoSync(sync, wait);
}

extern int framesyncflag;
bool mCoreSyncProduceAudio(struct mCoreSync* sync, const struct blip_t* buf, size_t samples) {
	if (!sync) {
		return true;
	}
    
	// thread remove
	//printf("ProduceAudio\n");
	if(framesyncflag==0) return true;
	
	
	size_t produced = blip_samples_avail(buf);
	size_t producedNew = produced;
	while (sync->audioWait && producedNew >= samples) {
		ConditionWait(&sync->audioRequiredCond, &sync->audioBufferMutex);
		produced = producedNew;
		producedNew = blip_samples_avail(buf);
	}
	MutexUnlock(&sync->audioBufferMutex);
	return producedNew != produced;
}

void mCoreSyncLockAudio(struct mCoreSync* sync) {
	if (!sync) {
		return;
	}
	
	if(framesyncflag==0) return true;
	
	
	MutexLock(&sync->audioBufferMutex);
}

void mCoreSyncUnlockAudio(struct mCoreSync* sync) {
	if (!sync) {
		return;
	}
	if(framesyncflag==0) return true;

	MutexUnlock(&sync->audioBufferMutex);
}

void mCoreSyncConsumeAudio(struct mCoreSync* sync) {
	if (!sync) {
		return;
	}
	if(framesyncflag==0) return true;
	
	// thread remove
	//printf("ConsumeAudio\n");		
	
	ConditionWake(&sync->audioRequiredCond);
	MutexUnlock(&sync->audioBufferMutex);
}
