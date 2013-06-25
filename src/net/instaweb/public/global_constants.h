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

// Author: pradnya@google.com (Pradnya Karbhari)

// Header file that includes global constants to be used in net/instaweb.

#ifndef NET_INSTAWEB_PUBLIC_GLOBAL_CONSTANTS_H_
#define NET_INSTAWEB_PUBLIC_GLOBAL_CONSTANTS_H_

namespace {

// Time of day used in Chromium when running javascript in deterministic mode
// (with flag --no-js-randomness). We use the same time of day for slurping,
// validation and measurement in order to maintain consistency.
static const double kChromiumTimeOfDay = 1204251968254LL;

const char kModPagespeedHeader[] = "X-Mod-Pagespeed";
const char kPageSpeedHeader[] = "X-Page-Speed";

// String added to all subrequests (with version string).
const char kModPagespeedSubrequestUserAgent[] = "mod_pagespeed";

const char kWPTUserAgentIdentifier[] = "PTST";

// The name of the header used to specify the rewriters that were
// applied to the current request.
const char kPsaRewriterHeader[] = "X-PSA-Rewriter";

// The name of the header that pubilshers can use to send the time when the
// cacheable content on the page was last modified.  This is used by
// prioritize_visible_content filter to invalidate its cache.
const char kPsaLastModified[] = "X-PSA-Last-Modified";

// Header whose existence indicates that this is a purge request to
// and external/internal caching layer and hence should not be
// allowed to initiate yet another purge request (which could result in
// an infinite loop).
const char kPsaPurgeRequest[] = "X-PSA-Purge-Request";

// Header used to specify a comma-separated list of optimization-related
// capabilities which are to be supported in the response. For e.g. "ii"
// could mean image inlining is supported. These are used by the
// downstream caching layer to communicate to the pagespeed server,
// the optimizations/capabilties that are going to be factored into
// the cache's fragmentation logic.
const char kPsaCapabilityList[] = "PS-CapabilityList";

// Noscript element that redirects to PageSpeed=noscript.  This is applied
// when a filter that inserts custom javascript is enabled.
const char kNoScriptRedirectFormatter[] =
    "<noscript><meta HTTP-EQUIV=\"refresh\" content=\"0;url='%s'\" />"
    "<style><!--table,div,span,font,p{display:none} --></style>"
    "<div style=\"display:block\">Please click <a href=\"%s\">here</a> "
    "if you are not redirected within a few seconds.</div></noscript>";

// Link tag to be inserted on noscript redirect so that original URL is
// considered canonical.
const char kLinkRelCanonicalFormatter[] =
    "<link rel=\"canonical\" href=\"%s\"/>";

}  // namespace

#endif  // NET_INSTAWEB_PUBLIC_GLOBAL_CONSTANTS_H_
