/*
 * Copyright (C) 2010 The Android Open Source Project
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

#ifndef __DRM_MANAGER_CLIENT_WRAPPER_H__
#define __DRM_MANAGER_CLIENT_WRAPPER_H__

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __cplusplus
    typedef struct android::DrmManagerClient DrmManagerClient; /* opaque */
    typedef struct android::DecryptHandle DecryptHandle;
#else
    typedef struct DrmManagerClient DrmManagerClient; /* opaque */
    typedef struct DecryptHandle DecryptHandle;
#endif

    DrmManagerClient* create_DrmManagerClient() ;
    void destroy_DrmManagerClient(DrmManagerClient* client);
    DecryptHandle* openDecryptSession_fd( DrmManagerClient* client, int fd, int offset, int length);
    DecryptHandle* openDecryptSession_uri( DrmManagerClient* client, char* uri);
    int closeDecryptSession(DrmManagerClient* client, DecryptHandle* decryptHandle);
    int consumeRights(DrmManagerClient* client, DecryptHandle* decryptHandle, int action /*, bool reserve*/);// because no bool in c language.
    long getContentSize(DrmManagerClient* client, DecryptHandle* decryptHandle);
    long pread_drm(DrmManagerClient* client, DecryptHandle* decryptHandle, void* buffer, long numBytes, long offset);
    void getFilePath(int fd, char* path);
    int isDcf_fd(int fd);
    int isDcf_path(const char* path);
    int isDcf_path_fd(const char* path, int fd);

#ifdef __cplusplus
}
#endif

#endif /* __DRM_MANAGER_CLIENT_WRAPPER_H__ */

