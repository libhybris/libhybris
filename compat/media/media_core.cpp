/*
 * Copyright (C) 2016 Canonical Ltd
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
 *
 * Authored by: Simon Fels <simon.fels@canonical.com>
 */

unsigned int hybris_media_get_version(void)
{
	/* The version number here will be bumped when the ABI of the
	 * media compatibility layer changes. This version number will
	 * be used by clients to track newly added functionality.
	 *
	 * If new functionality is added the client side is responsible
	 * to continue working on platforms where the new functionality
	 * is not yet available.
	 *
	 * Changelog:
	 * 1:
	 *  - Introduction of the new versioning approach
	 *  - MediaCodecSource support for Android 5.x based platforms
	 *  - Wrappers for AMessage, MediaBuffer, MediaMetaData etc. to
	 *    support MediaCodecSource implementation
	 */
	return 1;
}
