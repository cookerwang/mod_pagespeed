/*
 * Copyright 2010 Google Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

// Author: jmaessen@google.com (Jan Maessen)

#ifndef PAGESPEED_KERNEL_BASE_NAMED_LOCK_MANAGER_H_
#define PAGESPEED_KERNEL_BASE_NAMED_LOCK_MANAGER_H_

#include "pagespeed/kernel/base/basictypes.h"
#include "pagespeed/kernel/base/string.h"
#include "pagespeed/kernel/base/string_util.h"

namespace net_instaweb {

class Function;

class NamedLock {
 public:
  // Destructors of extending classes must unlock the lock if held on destruct.
  virtual ~NamedLock();

  // If lock is held, return false, otherwise lock and return true.
  // Non-blocking.  Note that implementations of this and other similar 'try'
  // routines are permitted to return false conservatively.  TryLock must
  // *eventually* succeed if called repeatedly on an unheld lock, however.
  virtual bool TryLock() = 0;

  // Wait bounded amount of time to take lock, otherwise return false.
  virtual bool LockTimedWait(int64 wait_ms) = 0;

  // Return immediately.  Wait wait_ms to take lock, invoke callback with lock
  // held.  On timeout, cancel callback.
  virtual void LockTimedWait(int64 wait_ms, Function* callback) = 0;

  // ...StealOld versions of locking routines steal the lock if its current
  // holder has locked it for more than timeout_ms.  *WARNING* If you use
  // any ...StealOld methods, your lock becomes "best-effort" and there may
  // be multiple workers in a critical section! *WARNING*

  // TryLockStealOld immediately attempts to lock the lock, succeeding and
  // returning true if the lock is unlocked or the lock can be stolen from the
  // current holder.  Otherwise return false.  cf TryLock() for other caveats.
  // Non-blocking.
  virtual bool TryLockStealOld(int64 timeout_ms) = 0;

  // LockTimedWaitStealOld without a callback will block for up to
  // wait_ms until the lock is unlocked, then lock it and return true.
  // If the current lock holder has locked it for more than
  // timeout_ms, the lock is "stolen" (re-locked by the caller).  If
  // wait_ms passes without the lock being unlocked or stolen, false
  // (failed to lock) is returned.
  //
  // Note that even if wait_ms > timeout_ms, callback->Cancel() may be
  // called if there are multiple concurrent attempts to take the
  // lock.
  virtual bool LockTimedWaitStealOld(int64 wait_ms, int64 timeout_ms) = 0;

  // LockTimedWaitStealOld with a callback returns immediately.  The
  // callback is Run if the lock can be obtained within wait_ms.  If
  // the current lock holder has locked it for more than timeout_ms,
  // the lock is "stolen" (re-locked by the caller and the callback is
  // called) and again the callback is Run.  If wait_ms passes without
  // the lock being unlocked or stolen, the callback's Cancel method
  // is called.
  //
  // Note that even if wait_ms > timeout_ms, callback->Cancel() may be
  // called if there are multiple concurrent attempts to take the
  // lock.
  virtual void LockTimedWaitStealOld(int64 wait_ms, int64 timeout_ms,
                                     Function* callback) = 0;

  // Relinquish lock.  Non-blocking.
  virtual void Unlock() = 0;

  // Returns true if this lock is held by this particular lock object.
  virtual bool Held() = 0;

  // The name the lock was created with, for debugging/logging purposes.
  virtual GoogleString name() = 0;
};

// A named_lock_manager provides global locks named by strings (with the same
// naming limitations in general as file names).  They provide a fairly rich
// api, with blocking and try versions and various timeout / steal behaviors.
class NamedLockManager {
 public:
  virtual ~NamedLockManager();
  virtual NamedLock* CreateNamedLock(const StringPiece& name) = 0;
};

}  // namespace net_instaweb

#endif  // PAGESPEED_KERNEL_BASE_NAMED_LOCK_MANAGER_H_
