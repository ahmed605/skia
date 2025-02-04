/*
 * Copyright 2015 Xamarin Inc.
 * Copyright 2017 Microsoft Corporation. All rights reserved.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "include/core/SkCanvas.h"
#include "include/core/SkPicture.h"
#include "include/xamarin/SkManagedDrawable.h"

SkManagedDrawable::Procs SkManagedDrawable::fProcs;

void SkManagedDrawable::setProcs(SkManagedDrawable::Procs procs) {
    fProcs = procs;
}

SkManagedDrawable::SkManagedDrawable(void* context) {
    fContext = context;
}
SkManagedDrawable::~SkManagedDrawable() {
    if (!fProcs.fDestroy) return;
    fProcs.fDestroy(this, fContext);
}

void SkManagedDrawable::onDraw(SkCanvas* canvas) {
    if (!fProcs.fDraw) return;
    fProcs.fDraw(this, fContext, canvas);
}
SkRect SkManagedDrawable::onGetBounds() {
    SkRect rect;
    if (fProcs.fGetBounds)
        fProcs.fGetBounds(this, fContext, &rect);
    return rect;
}
size_t SkManagedDrawable::onApproximateBytesUsed() {
    if (!fProcs.fApproximateBytesUsed) return 0;
    return fProcs.fApproximateBytesUsed(this, fContext);
}
sk_sp<SkPicture> SkManagedDrawable::onMakePictureSnapshot() {
    if (!fProcs.fMakePictureSnapshot) return nullptr;
    return fProcs.fMakePictureSnapshot(this, fContext);
}
