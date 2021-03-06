/*
 * Copyright 2014 Google Inc.
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

// Author: jmaessen@google.com (Jan-Willem Maessen)

#include "net/instaweb/rewriter/public/mobilize_label_filter.h"

#include "net/instaweb/rewriter/public/add_ids_filter.h"
#include "net/instaweb/rewriter/public/rewrite_driver.h"
#include "net/instaweb/rewriter/public/rewrite_options.h"
#include "net/instaweb/rewriter/public/rewrite_test_base.h"
#include "pagespeed/kernel/base/gtest.h"
#include "pagespeed/kernel/base/mock_message_handler.h"
#include "pagespeed/kernel/base/scoped_ptr.h"
#include "pagespeed/kernel/base/statistics.h"
#include "pagespeed/kernel/base/stdio_file_system.h"
#include "pagespeed/kernel/base/string.h"
#include "pagespeed/kernel/base/string_util.h"
#include "pagespeed/kernel/html/html_parse_test_base.h"

namespace net_instaweb {

namespace {

const char kTestDataDir[] = "/net/instaweb/rewriter/testdata/";
const char kOriginal[] = "mobilize_test.html";
const char kOriginalHtml5[] = "mobilize_test_html5.html";
const char kOriginalLabeled[] = "mobilize_test_labeled.html";
const char kOriginalHtml5Labeled[] = "mobilize_test_html5_labeled.html";

class MobilizeLabelFilterTest : public RewriteTestBase {
 protected:
  MobilizeLabelFilterTest() {}

  virtual void SetUp() {
    RewriteTestBase::SetUp();
    add_ids_filter_.reset(new AddIdsFilter(rewrite_driver()));
    label_filter_.reset(new MobilizeLabelFilter(rewrite_driver()));
    options()->set_mob_always(true);
    options()->set_mob_nav_server_side(true);
    html_parse()->AddFilter(add_ids_filter_.get());
    html_parse()->AddFilter(label_filter_.get());
    SetHtmlMimetype();
    Statistics* stats = statistics();
    pages_labeled_ =
        stats->GetVariable(MobilizeLabelFilter::kPagesLabeled);
    pages_role_added_ =
        stats->GetVariable(MobilizeLabelFilter::kPagesRoleAdded);
    navigational_roles_ =
        stats->GetVariable(MobilizeLabelFilter::kNavigationalRoles);
    header_roles_ =
        stats->GetVariable(MobilizeLabelFilter::kHeaderRoles);
    content_roles_ =
        stats->GetVariable(MobilizeLabelFilter::kContentRoles);
    marginal_roles_ =
        stats->GetVariable(MobilizeLabelFilter::kMarginalRoles);
    divs_unlabeled_ =
        stats->GetVariable(MobilizeLabelFilter::kDivsUnlabeled);
    ambiguous_role_labels_ =
        stats->GetVariable(MobilizeLabelFilter::kAmbiguousRoleLabels);
  }

  void EnableVerbose() {
    options()->set_log_mobilization_samples(true);
    EnableDebug();
  }

  // Remove data-mobile-role labeling from a labeled document
  GoogleString Unlabel(StringPiece labeled) {
    GoogleString result;
    labeled.CopyToString(&result);
    GlobalEraseBracketedSubstring(" data-mobile-role=\"", "\"", &result);
    GlobalEraseBracketedSubstring("<!--id: ", "-->", &result);
    GlobalReplaceSubstring("<!--No nodes labeled for mobilization-->", "",
                           &result);
    GlobalEraseBracketedSubstring(" id=\"PageSpeed-", "\"", &result);
    GlobalEraseBracketedSubstring("<script type=\"text/javascript\">",
                                  "</script>", &result);
    return result;
  }

  // Remove percentages and previous content bytes, which are very
  // input-sensitive, from output buffer so that we just check raw statistics
  // counts.
  void RemoveRedundantDataFromOutputBuffer() {
    GlobalEraseBracketedSubstring(
        "PreviousTagPercent:", ", ", &output_buffer_);
    GlobalEraseBracketedSubstring(
        "PreviousContentBytes:", ", ", &output_buffer_);
    GlobalEraseBracketedSubstring(
        "PreviousContentPercent:", ", ", &output_buffer_);
    GlobalEraseBracketedSubstring(
        "PreviousNonBlankBytes:", ", ", &output_buffer_);
    GlobalEraseBracketedSubstring(
        "PreviousNonBlankPercent:", ", ", &output_buffer_);
    GlobalEraseBracketedSubstring(
        "ContainedTagPercent:", ", ", &output_buffer_);
    GlobalEraseBracketedSubstring(
        "ContainedContentPercent:", ", ", &output_buffer_);
    GlobalEraseBracketedSubstring(
        "ContainedNonBlankPercent", ", ", &output_buffer_);
    GlobalReplaceSubstring("-->", ", -->", &output_buffer_);
    GlobalEraseBracketedSubstring("div percent:", ", ", &output_buffer_);
    GlobalEraseBracketedSubstring("h1 percent:", ", ", &output_buffer_);
    GlobalEraseBracketedSubstring("section percent:", ", ", &output_buffer_);
    GlobalReplaceSubstring(", -->", "-->", &output_buffer_);
  }

  scoped_ptr<AddIdsFilter> add_ids_filter_;
  scoped_ptr<MobilizeLabelFilter> label_filter_;
  Variable* pages_labeled_;
  Variable* pages_role_added_;
  Variable* navigational_roles_;
  Variable* header_roles_;
  Variable* content_roles_;
  Variable* marginal_roles_;
  Variable* divs_unlabeled_;
  Variable* ambiguous_role_labels_;

 private:
  DISALLOW_COPY_AND_ASSIGN(MobilizeLabelFilterTest);
};

TEST_F(MobilizeLabelFilterTest, AlreadyLabeled) {
  StdioFileSystem filesystem;
  GoogleString html5_filename =
      StrCat(GTestSrcDir(), kTestDataDir, kOriginalHtml5);
  GoogleString html5_contents;
  ASSERT_TRUE(filesystem.ReadFile(
      html5_filename.c_str(), &html5_contents, message_handler()));
  // Classify fully, compare against gold labeling.
  // Note that changes are fairly minimal.
  GoogleString labeled_filename =
      StrCat(GTestSrcDir(), kTestDataDir, kOriginalHtml5Labeled);
  GoogleString labeled_contents;
  ASSERT_TRUE(filesystem.ReadFile(
      labeled_filename.c_str(), &labeled_contents, message_handler()));
  ValidateExpected("already_labeled_adding_labels",
                   html5_contents, labeled_contents);
  EXPECT_EQ(1, pages_labeled_->Get());
  EXPECT_EQ(1, pages_role_added_->Get());
  EXPECT_EQ(2, navigational_roles_->Get());
  EXPECT_EQ(2, header_roles_->Get());
  EXPECT_EQ(3, content_roles_->Get());
  EXPECT_EQ(2, marginal_roles_->Get());
  EXPECT_EQ(0, ambiguous_role_labels_->Get());
  EXPECT_EQ(11, divs_unlabeled_->Get());
}

TEST_F(MobilizeLabelFilterTest, Html5TagsInHead) {
  const char kOutputHtml[] =
      "<head>\n"
      "<menu id=\"PageSpeed-0-0\" data-mobile-role=\"navigational\">"
      "Now treated as a menu</menu>\n"
      "<header id=\"PageSpeed-0-1\" data-mobile-role=\"header\">"
      "<h1>Also labeled</h1></header>\n"
      "<article id=\"PageSpeed-0-2\" data-mobile-role=\"content\">"
      "Still labeled</article>\n"
      "<footer id=\"PageSpeed-0-3\" data-mobile-role=\"marginal\">"
      "Also labeled</footer>\n"
      "</head>\n"
      "<script type=\"text/javascript\">"
      "pagespeedHeaderIds=['PageSpeed-0-1'];\n"
      "pagespeedNavigationalIds=['PageSpeed-0-0'];\n"
      "pagespeedContentIds=['PageSpeed-0-2'];\n"
      "pagespeedMarginalIds=['PageSpeed-0-3'];\n"
      "</script>";
  ValidateExpected("html5_tags_in_head",
                   Unlabel(kOutputHtml), kOutputHtml);
  EXPECT_EQ(1, pages_labeled_->Get());
  EXPECT_EQ(0, pages_role_added_->Get());
}

TEST_F(MobilizeLabelFilterTest, NoLabelableContent) {
  EnableDebug();
  const char kOutputHtml[] =
      "<body><p>\n"
      "Just a paragraph of plain old content.  "
      "Nothing to label here!"
      "</p>\n"
      "<!--No nodes labeled for mobilization-->";
  ValidateExpected("no_labelable_content",
                   Unlabel(kOutputHtml), kOutputHtml);
  EXPECT_EQ(1, pages_labeled_->Get());
  EXPECT_EQ(0, pages_role_added_->Get());
}

TEST_F(MobilizeLabelFilterTest, TinyCount) {
  EnableDebug();
  const char kOutputHtml[] =
      "<div role='header' id=\"PageSpeed-0\" data-mobile-role=\"header\">"
      "  Hello there,"
      " <a href='http://theworld.com/'>World</a></div>"
      "<!--id: PageSpeed-0,"
      " role: header,"
      " ElementTagDepth: 1,"
      " ContainedTagDepth: 2,"       // <a> tag
      " ContainedTagRelativeDepth: 1,"
      " ContainedTagCount: 2,"       // Includes <div> itself.
      " ContainedTagPercent: 100.00,"
      " ContainedContentBytes: 17,"  // Whitespace before <a> ignored.
      " ContainedContentPercent: 100.00,"
      " ContainedNonBlankBytes: 16,"
      " ContainedNonBlankPercent: 100.00,"
      " ContainedAContentBytes: 5,"
      " ContainedAContentLocalPercent: 29.41,"
      " ContainedNonAContentBytes: 12,"
      " head: 1,"
      " a count: 1,"
      " a percent: 100.00,"
      " div count: 1,"
      " div percent: 100.00-->\n"
      "<script type=\"text/javascript\">"
      "pagespeedHeaderIds=['PageSpeed-0'];\n"
      "</script>";
  ValidateExpected("Small count nav",
                   Unlabel(kOutputHtml), kOutputHtml);
  EXPECT_EQ(1, pages_labeled_->Get());
  EXPECT_EQ(1, pages_role_added_->Get());
  EXPECT_EQ(0, navigational_roles_->Get());
  EXPECT_EQ(1, header_roles_->Get());
  EXPECT_EQ(0, content_roles_->Get());
  EXPECT_EQ(0, marginal_roles_->Get());
  EXPECT_EQ(0, ambiguous_role_labels_->Get());
  EXPECT_EQ(0, divs_unlabeled_->Get());
}

TEST_F(MobilizeLabelFilterTest, TinyCountNbsp) {
  EnableDebug();
  const char kOutputHtml[] =
      "<div role='header' id=\"PageSpeed-0\" data-mobile-role=\"header\">"
      "  &nbsp;Hello&nbsp;there,&nbsp;&nbsp;  "
      " <a href='http://theworld.com/'>World</a></div>"
      "<!--id: PageSpeed-0,"
      " role: header,"
      " ElementTagDepth: 1,"
      " ContainedTagDepth: 2,"       // <a> tag
      " ContainedTagRelativeDepth: 1,"
      " ContainedTagCount: 2,"       // Includes <div> itself.
      " ContainedTagPercent: 100.00,"
      " ContainedContentBytes: 17,"  // Whitespace before <a> ignored.
      " ContainedContentPercent: 100.00,"
      " ContainedNonBlankBytes: 16,"
      " ContainedNonBlankPercent: 100.00,"
      " ContainedAContentBytes: 5,"
      " ContainedAContentLocalPercent: 29.41,"
      " ContainedNonAContentBytes: 12,"
      " head: 1,"
      " a count: 1,"
      " a percent: 100.00,"
      " div count: 1,"
      " div percent: 100.00-->\n"
      "<script type=\"text/javascript\">"
      "pagespeedHeaderIds=['PageSpeed-0'];\n"
      "</script>";
  ValidateExpected("Small count nav",
                   Unlabel(kOutputHtml), kOutputHtml);
  EXPECT_EQ(1, pages_labeled_->Get());
  EXPECT_EQ(1, pages_role_added_->Get());
  EXPECT_EQ(0, navigational_roles_->Get());
  EXPECT_EQ(1, header_roles_->Get());
  EXPECT_EQ(0, content_roles_->Get());
  EXPECT_EQ(0, marginal_roles_->Get());
  EXPECT_EQ(0, ambiguous_role_labels_->Get());
  EXPECT_EQ(0, divs_unlabeled_->Get());
}

TEST_F(MobilizeLabelFilterTest, ImgInsideAndOutsideA) {
  EnableDebug();
  const char kOutputHtml[] =
      "<div role='content' id=\"PageSpeed-0\" data-mobile-role=\"header\">"
      " <img src='a.png'>"
      " <img src='b.jpg'>"
      " <a href='http://theworld.com/'><img src='world.gif'></a></div>"
      "<!--id: PageSpeed-0,"
      " role: header,"
      " ElementTagDepth: 1,"
      " ContainedTagDepth: 3,"       // <a><img></a>
      " ContainedTagRelativeDepth: 2,"
      " ContainedTagCount: 5,"       // Includes <div> itself.
      " ContainedTagPercent: 100.00,"
      " ContainedAImgTag: 1,"
      " ContainedAImgLocalPercent: 33.33,"
      " ContainedNonAImgTag: 2,"
      " content: 1,"
      " a count: 1,"
      " a percent: 100.00,"
      " div count: 1,"
      " div percent: 100.00,"
      " img count: 3,"
      " img percent: 100.00-->\n"
      "<script type=\"text/javascript\">"
      "pagespeedHeaderIds=['PageSpeed-0'];\n"
      "</script>";
  ValidateExpected("Small count nav",
                   Unlabel(kOutputHtml), kOutputHtml);
  EXPECT_EQ(1, pages_labeled_->Get());
  EXPECT_EQ(1, pages_role_added_->Get());
  EXPECT_EQ(0, navigational_roles_->Get());
  EXPECT_EQ(1, header_roles_->Get());
  EXPECT_EQ(0, content_roles_->Get());
  EXPECT_EQ(0, marginal_roles_->Get());
  EXPECT_EQ(0, ambiguous_role_labels_->Get());
  EXPECT_EQ(0, divs_unlabeled_->Get());
}

TEST_F(MobilizeLabelFilterTest, DontCrashWithUnicodeId) {
  const char kOutputHtml[] =
      "<header id='g\xc5\x82\xc3\xb3wna' data-mobile-role=\"header\">"
      "Header</header>\n"
      "<script type=\"text/javascript\">"
      "pagespeedHeaderIds=['g\xc5\x82\xc3\xb3wna'];\n"
      "</script>";
  ValidateExpected("Unicode id", Unlabel(kOutputHtml), kOutputHtml);
}

TEST_F(MobilizeLabelFilterTest, DontCrashWithEmptyId) {
  const char kOutputHtml[] =
      "<header id='' data-mobile-role=\"header\">Header</header>\n"
      "<script type=\"text/javascript\">"
      "pagespeedHeaderIds=[''];\n"
      "</script>";
  ValidateExpected("Empty id", Unlabel(kOutputHtml), kOutputHtml);
}

TEST_F(MobilizeLabelFilterTest, DontCrashWithBlankId) {
  const char kOutputHtml[] =
      "<header id data-mobile-role=\"header\">Header</header>\n"
      "<script type=\"text/javascript\">"
      "pagespeedHeaderIds=[''];\n"
      "</script>";
  ValidateExpected("Blank id", Unlabel(kOutputHtml), kOutputHtml);
}

TEST_F(MobilizeLabelFilterTest, InternalQuotesAndSpacesInId) {
  const char kOutputHtml[] =
      "<header id=\"'Quotes'\\slashes\" data-mobile-role=\"header\">"
      "Header</header>\n"
      "<script type=\"text/javascript\">"
      "pagespeedHeaderIds=['\\'Quotes\\'\\\\slashes'];\n"
      "</script>";
  ValidateExpected("Quotes\\slashes in id", Unlabel(kOutputHtml), kOutputHtml);
}

TEST_F(MobilizeLabelFilterTest, CloseScriptInId) {
  const char kOutputHtml[] =
      "<header id='</script>' data-mobile-role=\"header\">Header</header>\n"
      "<script type=\"text/javascript\">"
      "pagespeedHeaderIds=['<\\/script>'];\n"
      "</script>";
  ValidateExpected("Close script in id", Unlabel(kOutputHtml), kOutputHtml);
}

TEST_F(MobilizeLabelFilterTest, DontCrashWithFlush) {
  // Note that we cannot remove unused ids inserted before the flush.
  const char kBody1[] =
      "<html><head></head><body>\n"
      "<div id=\"PageSpeed-1\">\n"
      "<div role='nav' id=\"PageSpeed-1-0\"><a href='http://theworld.com/'>\n"
      "Hello, World\n"
      "</a></div>";
  const char kBody2[] =
      "</div>\n"
      "<script type=\"text/javascript\">"
      "pagespeedNavigationalIds=['PageSpeed-1'];\n"
      "</script>"
      "</body></html>";
  SetupWriter();
  rewrite_driver()->StartParse(kTestDomain);
  rewrite_driver()->ParseText(Unlabel(kBody1));
  rewrite_driver()->Flush();
  rewrite_driver()->ParseText(Unlabel(kBody2));
  rewrite_driver()->FinishParse();
  GoogleString expected = StrCat(kBody1, kBody2);
  EXPECT_STREQ(expected, output_buffer_);
}

TEST_F(MobilizeLabelFilterTest, DontCrashWithFlushAndDebug) {
  EnableDebug();
  // We can't insert helpful comments because the tags aren't rewritable
  // anymore.  Note that this is true even for the spanning <div>, where we
  // arguably ought to be able to insert *after* the closing tag as it's still
  // in the flush window.
  const char kBody1[] =
      "<html><head></head><body>\n"
      "<div id=\"PageSpeed-1\">\n"
      "<div role='nav' id=\"PageSpeed-1-0\"><a href='http://theworld.com/'>\n"
      "Hello, World\n"
      "</a></div>";
  const char kBody2[] =
      "</div>\n"
      "<!--id: PageSpeed-1,"
      " role: navigational,"
      " ElementTagDepth: 1,"
      " ContainedTagDepth: 3,"
      " ContainedTagRelativeDepth: 2,"
      " ContainedTagCount: 3,"
      " ContainedTagPercent: 100.00,"
      " ContainedContentBytes: 12,"
      " ContainedContentPercent: 100.00,"
      " ContainedNonBlankBytes: 11,"
      " ContainedNonBlankPercent: 100.00,"
      " ContainedAContentBytes: 12,"
      " ContainedAContentLocalPercent: 100.00,"
      " a count: 1,"
      " a percent: 100.00,"
      " div count: 2,"
      " div percent: 100.00-->"
      "<!--id: PageSpeed-1-0,"
      " ElementTagDepth: 2,"
      " PreviousTagCount: 1,"
      " PreviousTagPercent: 33.33,"
      " ContainedTagDepth: 3,"
      " ContainedTagRelativeDepth: 1,"
      " ContainedTagCount: 2,"
      " ContainedTagPercent: 66.67,"
      " ContainedContentBytes: 12,"
      " ContainedContentPercent: 100.00,"
      " ContainedNonBlankBytes: 11,"
      " ContainedNonBlankPercent: 100.00,"
      " ContainedAContentBytes: 12,"
      " ContainedAContentLocalPercent: 100.00,"
      " nav: 1,"
      " a count: 1,"
      " a percent: 100.00,"
      " div count: 1,"
      " div percent: 50.00,"
      " parent role is navigational-->"
      "<script type=\"text/javascript\">"
      "pagespeedNavigationalIds=['PageSpeed-1'];\n"
      "</script>"
      "</body></html>";
  SetupWriter();
  rewrite_driver()->StartParse(kTestDomain);
  rewrite_driver()->ParseText(Unlabel(kBody1));
  rewrite_driver()->Flush();
  rewrite_driver()->ParseText(Unlabel(kBody2));
  rewrite_driver()->FinishParse();
  GoogleString expected = StrCat(kBody1, kBody2);
  EXPECT_STREQ(expected, output_buffer_);
}

TEST_F(MobilizeLabelFilterTest, DontCrashWithMarginalChildOfNav) {
  const char kOutputHtml[] =
      "<div id='a' data-mobile-role=\"navigational\">\n"
      "  <ul id='b'>\n"
      "    <li><a href='/'>Drive</a></li>\n"
      "  </ul>\n"
      "  <ul id='c' data-mobile-role=\"marginal\">\n"
      "    <li><a href='R8'>R8</a></li>\n"
      "  </ul>\n"
      "</div>\n"
      "<script type=\"text/javascript\">"
      "pagespeedNavigationalIds=['a'];\n"
      "pagespeedMarginalIds=['c'];\n"
      "</script>";
  ValidateExpected("DontCrashWithMarginalChild",
                   Unlabel(kOutputHtml), kOutputHtml);
}

TEST_F(MobilizeLabelFilterTest, MarginalPropagation) {
  // Test that marginal content gets labeled as such, and the
  // labels get propagated up the DOM (but only as far as the
  // outermost parent that isn't otherwise labeled).
  const char kOutputHtml[] =
      "<div>\n"
      " <div data-mobile-role='header' id=\"PageSpeed-0-0\">header</div>\n"
      " <div id=\"PageSpeed-0-1\" data-mobile-role=\"content\">\n"
      "  <p>Content</p>\n"
      "  <p>More content</p>\n"
      "  <p>Still more content</p>\n"
      "  <p>Are we still here? This is really quite a lot of content.</p>\n"
      "  <p>Are we still here? This is really quite a lot of content.</p>\n"
      "  <p>Are we still here? This is really quite a lot of content.</p>\n"
      "  <p>Are we still here? This is really quite a lot of content.</p>\n"
      "  <p>Are we still here? This is really quite a lot of content.</p>\n"
      "  <p>Are we still here? This is really quite a lot of content.</p>\n"
      "  <p>Are we still here? This is really quite a lot of content.</p>\n"
      "  <p>Are we still here? This is really quite a lot of content.</p>\n"
      "  <p>Are we still here? This is really quite a lot of content.</p>\n"
      "  <p>Are we still here? This is really quite a lot of content.</p>\n"
      "  <p>Are we still here? This is really quite a lot of content.</p>\n"
      "  <p>Are we still here? This is really quite a lot of content.</p>\n"
      "  <p>Are we still here? This is really quite a lot of content.</p>\n"
      "  <p>Are we still here? This is really quite a lot of content.</p>\n"
      "  <p>Are we still here? This is really quite a lot of content.</p>\n"
      "  <p>Are we still here? This is really quite a lot of content.</p>\n"
      "  <p>Are we still here? This is really quite a lot of content.</p>\n"
      "  <p>Are we still here? This is really quite a lot of content.</p>\n"
      "  <p>Are we still here? This is really quite a lot of content.</p>\n"
      "  <p>Are we still here? This is really quite a lot of content.</p>\n"
      "  <p>Are we still here? This is really quite a lot of content.</p>\n"
      "  <p>Are we still here? This is really quite a lot of content.</p>\n"
      "  <p>Are we still here? This is really quite a lot of content.</p>\n"
      "  <p>Are we still here? This is really quite a lot of content.</p>\n"
      "  <p>Are we still here? This is really quite a lot of content.</p>\n"
      "  <p>Are we still here? This is really quite a lot of content.</p>\n"
      "  <p>Are we still here? This is really quite a lot of content.</p>\n"
      "  <p>Are we still here? This is really quite a lot of content.</p>\n"
      "  <p>Are we still here? This is really quite a lot of content.</p>\n"
      "  <p>Are we still here? This is really quite a lot of content.</p>\n"
      "  <p>Are we still here? This is really quite a lot of content.</p>\n"
      "  <p>Are we still here? This is really quite a lot of content.</p>\n"
      "  <p>Are we still here? This is really quite a lot of content.</p>\n"
      "  <p>Are we still here? This is really quite a lot of content.</p>\n"
      "  <p>Are we still here? This is really quite a lot of content.</p>\n"
      "  <p>Are we still here? This is really quite a lot of content.</p>\n"
      "  <p>Are we still here? This is really quite a lot of content.</p>\n"
      "  <p>Are we still here? This is really quite a lot of content.</p>\n"
      "  <p>Are we still here? This is really quite a lot of content.</p>\n"
      "  <p>Are we still here? This is really quite a lot of content.</p>\n"
      "  <p>Are we still here? This is really quite a lot of content.</p>\n"
      "  <p>Are we still here? This is really quite a lot of content.</p>\n"
      "  <p>Are we still here? This is really quite a lot of content.</p>\n"
      "  <p>Are we still here? This is really quite a lot of content.</p>\n"
      "  <p>Are we still here? This is really quite a lot of content.</p>\n"
      "  <p>Are we still here? This is really quite a lot of content.</p>\n"
      "  <p>Are we still here? This is really quite a lot of content.</p>\n"
      "  <p>Are we still here? This is really quite a lot of content.</p>\n"
      "  <p>Are we still here? This is really quite a lot of content.</p>\n"
      "  <p>Are we still here? This is really quite a lot of content.</p>\n"
      "  <p>Are we still here? This is really quite a lot of content.</p>\n"
      "  <p>Are we still here? This is really quite a lot of content.</p>\n"
      "  <p>Are we still here? This is really quite a lot of content.</p>\n"
      "  <p>Are we still here? This is really quite a lot of content.</p>\n"
      "  <p>Are we still here? This is really quite a lot of content.</p>\n"
      "  <p>Are we still here? This is really quite a lot of content.</p>\n"
      " </div>\n"
      " <div id=\"PageSpeed-0-2\" data-mobile-role=\"marginal\">\n"
      "  A Marginal Title\n"
      "  <div role='footer'><a>footer</a></div>\n"
      "  <div role='junk'><a>junk</a></div>\n"
      "  <div><a>more junk</a></div>\n"
      " </div>\n"
      "</div>"
      "<script type=\"text/javascript\">"
      "pagespeedHeaderIds=['PageSpeed-0-0'];\n"
      "pagespeedContentIds=['PageSpeed-0-1'];\n"
      "pagespeedMarginalIds=['PageSpeed-0-2'];\n"
      "</script>";
  ValidateExpected("Marginal propagation",
                   Unlabel(kOutputHtml), kOutputHtml);
  EXPECT_EQ(1, pages_labeled_->Get());
  EXPECT_EQ(1, pages_role_added_->Get());
  EXPECT_EQ(0, navigational_roles_->Get());
  EXPECT_EQ(1, header_roles_->Get());
  EXPECT_EQ(1, content_roles_->Get());
  EXPECT_EQ(1, marginal_roles_->Get());
  EXPECT_EQ(0, ambiguous_role_labels_->Get());
  EXPECT_EQ(4, divs_unlabeled_->Get());
}

TEST_F(MobilizeLabelFilterTest, ParentPropagation) {
  options()->set_log_mobilization_samples(true);
  // Make sure an element all of whose children are labeled inherits the label,
  // and an element whose children's labels conflict does not.
  const char kOutputHtml[] =
      "<div>\n"  // One nav, one header, one content -> no label.
      " <header id=\"PageSpeed-0-0\" data-mobile-role=\"header\"></header>\n"
      " <nav id=\"PageSpeed-0-1\" data-mobile-role=\"navigational\"></nav>\n"
      "</div>\n"
      // Both children nav:
      "<div id=\"PageSpeed-1\" data-mobile-role=\"navigational\">\n"
      " <div>\n"  // Only child is nav, so nav.
      "  <nav></nav>\n"
      " </div>\n"
      " <nav></nav>\n"
      "</div>\n"
      "<script type=\"text/javascript\">"
      "pagespeedHeaderIds=['PageSpeed-0-0'];\n"
      "pagespeedNavigationalIds=['PageSpeed-0-1','PageSpeed-1'];\n"
      "</script>";
  ValidateExpected("Parent propagation",
                   Unlabel(kOutputHtml), kOutputHtml);
}

TEST_F(MobilizeLabelFilterTest, SmallCountNav) {
  EnableDebug();
  const char kOutputHtml[] =
      "<head></head><body>\n"
      "<div class='container' id=\"PageSpeed-1\""
      " data-mobile-role=\"navigational\">\n"
      " <a href='a'>a</a>\n"
      " <div class='menu' id='hdr' role='nav'>\n"
      "  <ul id=\"PageSpeed-hdr-0\">\n"
      "   <li><a href='n1'>nav 1</a></li>\n"
      "   <li><a href='n2'>nav 2</a></li>\n"
      "   <li><a href='n3'>nav 3</a></li>\n"
      "  </ul>"
      "<!--id: PageSpeed-hdr-0,"
      " ElementTagDepth: 3,"
      " PreviousTagCount: 3,"
      " PreviousTagPercent: 30.00,"
      " PreviousContentBytes: 1,"
      " PreviousContentPercent: 6.25,"
      " PreviousNonBlankBytes: 1,"
      " PreviousNonBlankPercent: 7.69,"
      " ContainedTagDepth: 5,"
      " ContainedTagRelativeDepth: 2,"
      " ContainedTagCount: 7,"
      " ContainedTagPercent: 70.00,"
      " ContainedContentBytes: 15,"
      " ContainedContentPercent: 93.75,"
      " ContainedNonBlankBytes: 12,"
      " ContainedNonBlankPercent: 92.31,"
      " ContainedAContentBytes: 15,"
      " ContainedAContentLocalPercent: 100.00,"
      " a count: 3,"
      " a percent: 75.00,"
      " li count: 3,"
      " li percent: 100.00,"
      " ul count: 1,"
      " ul percent: 100.00-->\n"
      " </div>"
      "<!--id: hdr,"
      " ElementTagDepth: 2,"
      " PreviousTagCount: 2,"
      " PreviousTagPercent: 20.00,"
      " PreviousContentBytes: 1,"
      " PreviousContentPercent: 6.25,"
      " PreviousNonBlankBytes: 1,"
      " PreviousNonBlankPercent: 7.69,"
      " ContainedTagDepth: 5,"
      " ContainedTagRelativeDepth: 3,"
      " ContainedTagCount: 8,"
      " ContainedTagPercent: 80.00,"
      " ContainedContentBytes: 15,"
      " ContainedContentPercent: 93.75,"
      " ContainedNonBlankBytes: 12,"
      " ContainedNonBlankPercent: 92.31,"
      " ContainedAContentBytes: 15,"
      " ContainedAContentLocalPercent: 100.00,"
      " hdr: 1,"
      " menu: 1,"
      " nav: 1,"
      " a count: 3,"
      " a percent: 75.00,"
      " div count: 1,"
      " div percent: 50.00,"
      " li count: 3,"
      " li percent: 100.00,"
      " ul count: 1,"
      " ul percent: 100.00-->\n"
      "</div>"
      "<!--id: PageSpeed-1,"
      " role: navigational,"
      " ElementTagDepth: 1,"
      " ContainedTagDepth: 5,"
      " ContainedTagRelativeDepth: 4,"
      " ContainedTagCount: 10,"
      " ContainedTagPercent: 100.00,"
      " ContainedContentBytes: 16,"
      " ContainedContentPercent: 100.00,"
      " ContainedNonBlankBytes: 13,"
      " ContainedNonBlankPercent: 100.00,"
      " ContainedAContentBytes: 16,"
      " ContainedAContentLocalPercent: 100.00,"
      " a count: 4,"
      " a percent: 100.00,"
      " div count: 2,"
      " div percent: 100.00,"
      " li count: 3,"
      " li percent: 100.00,"
      " ul count: 1,"
      " ul percent: 100.00-->\n"
      "<script type=\"text/javascript\">"
      "pagespeedNavigationalIds=['PageSpeed-1'];\n"
      "</script></body>";
  ValidateExpected("Small count nav",
                   Unlabel(kOutputHtml), kOutputHtml);
  EXPECT_EQ(1, pages_labeled_->Get());
  EXPECT_EQ(1, pages_role_added_->Get());
  EXPECT_EQ(1, navigational_roles_->Get());
  EXPECT_EQ(0, header_roles_->Get());
  EXPECT_EQ(0, content_roles_->Get());
  EXPECT_EQ(0, marginal_roles_->Get());
  EXPECT_EQ(0, ambiguous_role_labels_->Get());
  EXPECT_EQ(2, divs_unlabeled_->Get());
}

TEST_F(MobilizeLabelFilterTest, NoLabelInsideA) {
  // First, make sure we identify things correctly without <a>
  const char kOutputHtmlNoA[] =
      "<head></head><body>\n"
      "  <div role='header' id=\"PageSpeed-1\" data-mobile-role=\"header\">\n"
      "    <h1><img src='logo.gif'>Header!</h1></div>\n"
      " <div class='container' id=\"PageSpeed-2\""
      " data-mobile-role=\"navigational\">\n"
      "  <a href='a'>a</a>\n"
      "  <div class='menu' id='hdr' role='nav'>\n"
      "   <ul>\n"
      "    <li><a href='n1'>nav 1</a></li>\n"
      "    <li><a href='n2'>nav 2</a></li>\n"
      "    <li><a href='n3'>nav 3</a></li>\n"
      "   </ul>\n"
      "  </div>\n"
      " </div>\n"
      "<script type=\"text/javascript\">"
      "pagespeedHeaderIds=['PageSpeed-1'];\n"
      "pagespeedNavigationalIds=['PageSpeed-2'];\n"
      "</script></body>";
  ValidateExpected("Label not inside <a>",
                   Unlabel(kOutputHtmlNoA), kOutputHtmlNoA);
  // Now make sure that inside <a> we don't identify the header, but we do
  // identify the nav because it in turn contains nested links indicating that
  // the outer <a> was an error.
  const char kOutputHtmlWithA[] =
      "<head></head><body>\n"
      "<a href=top.html>\n"  // Now enclosed by a
      "  <div role='header'>\n"  // Not labeled => no id
      "    <h1><img src='logo.gif'>Header!</h1></div></a>\n"
      "<a href=menu.html>\n"  // Also enclosed by a
      " <div class='container' id=\"PageSpeed-2-0\""
      " data-mobile-role=\"navigational\">\n"
      "  <a href='a'>a</a>\n"  // But contains lots of a, so still label.
      "  <div class='menu' id='hdr' role='nav'>\n"
      "   <ul>\n"
      "    <li><a href='n1'>nav 1</a></li>\n"
      "    <li><a href='n2'>nav 2</a></li>\n"
      "    <li><a href='n3'>nav 3</a></li>\n"
      "   </ul>\n"
      "  </div>\n"
      " </div>\n"
      "</a>\n"
      "<script type=\"text/javascript\">"  // No header divs remain.
      "pagespeedNavigationalIds=['PageSpeed-2-0'];\n"
      "</script></body>";
  ValidateExpected("No label inside <a>",
                   Unlabel(kOutputHtmlWithA), kOutputHtmlWithA);
  EXPECT_EQ(2, pages_labeled_->Get());
  EXPECT_EQ(2, pages_role_added_->Get());
  EXPECT_EQ(2, navigational_roles_->Get());
  EXPECT_EQ(1, header_roles_->Get());
  EXPECT_EQ(0, content_roles_->Get());
  EXPECT_EQ(0, marginal_roles_->Get());
  EXPECT_EQ(0, ambiguous_role_labels_->Get());
  EXPECT_EQ(5, divs_unlabeled_->Get());
}

TEST_F(MobilizeLabelFilterTest, ConfiguredInclusionAndExclusion) {
  // Here we use HTML5 tags to create a document that would normally classify a
  // particular way, and use configuration to change the classification of those
  // tags.
  options()->set_mob_nav_classes("+ok,-no,yes");
  const char kOutputHtml[] =
      "<head></head><body>\n"
      " <header class='maybe ok yup' id=\"PageSpeed-1\""
      " data-mobile-role=\"navigational\">\n"
      "  <ul><li><a href='n1'>Actually navigational</a></ul>\n"
      " </header>\n"
      " <nav class='maybe ok yup' id='no' data-mobile-role=\"marginal\">\n"
      "  <nav id='no-a' data-mobile-role=\"navigational\">\n"
      "    Nested forced nav</nav>\n"
      "  <header class='yes' id='no-b' data-mobile-role=\"navigational\">\n"
      "    Overridden</header>\n"
      "  <ul><li><a href='n2'>Inherited non-navigational</a></ul>\n"
      " </nav>\n"
      " <nav class='yes no ok' id=\"PageSpeed-3\""
      " data-mobile-role=\"marginal\">\n"
      "  <ul><li><a href='n1'>Not navigational</a></ul>\n"
      " </nav>\n"
      " <em class='no' id='yes' data-mobile-role=\"navigational\">\n"
      "  Navigational\n"
      " </em>\n"
      " <script type=\"text/javascript\">"
      "pagespeedNavigationalIds=['PageSpeed-1','no-a','no-b','yes'];\n"
      "pagespeedMarginalIds=['no','PageSpeed-3'];\n"
      "</script>";
  ValidateExpected("Configured inclusion and exclusion",
                   Unlabel(kOutputHtml), kOutputHtml);
}

TEST_F(MobilizeLabelFilterTest, NavInsideHeader) {
  // A common pattern in sites is to have a header area with a logo and some
  // navigational content.  We'd like to flag the navigational content!
  EnableDebug();
  const char kOutputHtml[] =
      "<head></head><body>\n"
      " <header id=\"PageSpeed-1\" data-mobile-role=\"header\">\n"
      "  <img src='logo.gif'>\n"
      "  <ul id='nav_menu' data-mobile-role=\"navigational\">\n"
      "   <li><a href='about.html'>About us</a>\n"
      "   <li><a href='contact.html'>Contact</a>\n"
      "   <li><a href='faq.html'>FAQ</a>\n"
      "  </ul>"
      "<!--id: nav_menu,"
      " role: navigational,"
      " ElementTagDepth: 2,"
      " PreviousTagCount: 2,"
      " PreviousTagPercent: 22.22,"
      " ContainedTagDepth: 4,"
      " ContainedTagRelativeDepth: 2,"
      " ContainedTagCount: 7,"
      " ContainedTagPercent: 77.78,"
      " ContainedContentBytes: 18,"
      " ContainedContentPercent: 100.00,"
      " ContainedNonBlankBytes: 17,"
      " ContainedNonBlankPercent: 100.00,"
      " ContainedAContentBytes: 18,"
      " ContainedAContentLocalPercent: 100.00,"
      " menu: 1,"
      " nav: 1,"
      " a count: 3,"
      " a percent: 100.00,"
      " li count: 3,"
      " li percent: 100.00,"
      " ul count: 1,"
      " ul percent: 100.00,"
      " parent role is header-->\n"
      " </header>"
      "<!--id: PageSpeed-1,"
      " role: header,"
      " ElementTagDepth: 1,"
      " ContainedTagDepth: 4,"
      " ContainedTagRelativeDepth: 3,"
      " ContainedTagCount: 9,"
      " ContainedTagPercent: 100.00,"
      " ContainedContentBytes: 18,"
      " ContainedContentPercent: 100.00,"
      " ContainedNonBlankBytes: 17,"
      " ContainedNonBlankPercent: 100.00,"
      " ContainedAContentBytes: 18,"
      " ContainedAContentLocalPercent: 100.00,"
      " ContainedNonAImgTag: 1,"
      " a count: 3,"
      " a percent: 100.00,"
      " div count: 1,"
      " div percent: 100.00,"
      " img count: 1,"
      " img percent: 100.00,"
      " li count: 3,"
      " li percent: 100.00,"
      " ul count: 1,"
      " ul percent: 100.00-->\n"
      "<script type=\"text/javascript\">"
      "pagespeedHeaderIds=['PageSpeed-1'];\n"
      "pagespeedNavigationalIds=['nav_menu'];\n"
      "</script></body>";
  ValidateExpected("Nav inside header",
                   Unlabel(kOutputHtml), kOutputHtml);
  EXPECT_EQ(1, pages_labeled_->Get());
  EXPECT_EQ(1, pages_role_added_->Get());
  EXPECT_EQ(1, navigational_roles_->Get());
  EXPECT_EQ(1, header_roles_->Get());
  EXPECT_EQ(0, content_roles_->Get());
  EXPECT_EQ(0, marginal_roles_->Get());
  EXPECT_EQ(0, ambiguous_role_labels_->Get());
  EXPECT_EQ(0, divs_unlabeled_->Get());
}

TEST_F(MobilizeLabelFilterTest, Html5TagsInBody) {
  EnableDebug();
  // Just for clarity we include the labeled HTML without the sample comments
  // emitted by debug.  The input HTML is this with the data-mobile-role
  // annotations stripped out.
  const char kLabeledHtml[] =
      "<head></head><body>\n"
      "<nav data-mobile-role=\"navigational\">Labeled\n"
      "  <menu>unlabeled</menu>\n"
      "</nav>\n"
      "<menu data-mobile-role=\"navigational\">\n"
      "  Labeled</menu>\n"
      "<header data-mobile-role=\"header\">\n"
      "  <h1>Labeled</h1></header>\n"
      "<div id='body' data-mobile-role=\"content\">\n"
      "  <main>labeled\n"
      "    <article>\n"
      "      <section>unlabeled</section>\n"
      "    </article>\n"
      "  </main>\n"
      "  <article data-mobile-role=\"content\">also labeled</article>\n"
      "  <section data-mobile-role=\"content\">this too\n"
      "    <aside data-mobile-role=\"marginal\">\n"
      "      and this, it differs.</aside>\n"
      "  </section>\n"
      "</div>\n"
      "<aside data-mobile-role=\"marginal\">Labeled</aside>\n"
      "<footer data-mobile-role=\"marginal\">labeled\n"
      "  <menu data-mobile-role=\"navigational\">\n"
      "    navvy</menu>\n"
      "</footer>\n"
      "</body>";
  // Note how the HTML5 tags used for training / instant classification are
  // treated as divs in the instrumented data.
  const char kOutputHtml[] =
      "<head></head><body>\n"
      "<nav id=\"PageSpeed-1\" data-mobile-role=\"navigational\">Labeled\n"
      "  <menu id=\"PageSpeed-1-0\">unlabeled</menu>"
      "<!--id: PageSpeed-1-0,"
      " ElementTagDepth: 2,"
      " PreviousTagCount: 1,"
      " ContainedTagDepth: 2,"
      " ContainedTagRelativeDepth: 0,"
      " ContainedTagCount: 1,"
      " ContainedContentBytes: 9,"
      " ContainedNonBlankBytes: 9,"
      " ContainedNonAContentBytes: 9,"
      " div count: 1,"
      " parent role is navigational-->\n"
      "</nav>"
      "<!--id: PageSpeed-1,"
      " role: navigational,"
      " ElementTagDepth: 1,"
      " ContainedTagDepth: 2,"
      " ContainedTagRelativeDepth: 1,"
      " ContainedTagCount: 2,"
      " ContainedContentBytes: 16,"
      " ContainedNonBlankBytes: 16,"
      " ContainedNonAContentBytes: 16,"
      " div count: 2-->\n"
      "<menu id=\"PageSpeed-2\" data-mobile-role=\"navigational\">\n"
      "  Labeled</menu>"
      "<!--id: PageSpeed-2,"
      " role: navigational,"
      " ElementTagDepth: 1,"
      " PreviousTagCount: 2,"
      " ContainedTagDepth: 1,"
      " ContainedTagRelativeDepth: 0,"
      " ContainedTagCount: 1,"
      " ContainedContentBytes: 7,"
      " ContainedNonBlankBytes: 7,"
      " ContainedNonAContentBytes: 7,"
      " div count: 1-->\n"
      "<header id=\"PageSpeed-3\" data-mobile-role=\"header\">\n"
      "  <h1>Labeled</h1></header>"
      "<!--id: PageSpeed-3,"
      " role: header,"
      " ElementTagDepth: 1,"
      " PreviousTagCount: 3,"
      " ContainedTagDepth: 2,"
      " ContainedTagRelativeDepth: 1,"
      " ContainedTagCount: 2,"
      " ContainedContentBytes: 7,"
      " ContainedNonBlankBytes: 7,"
      " ContainedNonAContentBytes: 7,"
      " div count: 1,"
      " h1 count: 1-->\n"
      "<div id='body' data-mobile-role=\"content\">\n"
      "  <main id=\"PageSpeed-body-0\">labeled\n"
      "    <article id=\"PageSpeed-body-0-0\">\n"
      "      <section id=\"PageSpeed-body-0-0-0\">unlabeled</section>"
      "<!--id: PageSpeed-body-0-0-0,"
      " ElementTagDepth: 4,"
      " PreviousTagCount: 8,"
      " ContainedTagDepth: 4,"
      " ContainedTagRelativeDepth: 0,"
      " ContainedTagCount: 1,"
      " ContainedContentBytes: 9,"
      " ContainedNonBlankBytes: 9,"
      " ContainedNonAContentBytes: 9,"
      " section count: 1,"
      " parent role is content-->\n"
      "    </article>"
      "<!--id: PageSpeed-body-0-0,"
      " ElementTagDepth: 3,"
      " PreviousTagCount: 7,"
      " ContainedTagDepth: 4,"
      " ContainedTagRelativeDepth: 1,"
      " ContainedTagCount: 2,"
      " ContainedContentBytes: 9,"
      " ContainedNonBlankBytes: 9,"
      " ContainedNonAContentBytes: 9,"
      " div count: 1,"
      " section count: 1,"
      " parent role is content-->\n"
      "  </main>"
      "<!--id: PageSpeed-body-0,"
      " ElementTagDepth: 2,"
      " PreviousTagCount: 6,"
      " ContainedTagDepth: 4,"
      " ContainedTagRelativeDepth: 2,"
      " ContainedTagCount: 3,"
      " ContainedContentBytes: 16,"
      " ContainedNonBlankBytes: 16,"
      " ContainedNonAContentBytes: 16,"
      " div count: 2,"
      " section count: 1,"
      " parent role is content-->\n"
      "  <article id=\"PageSpeed-body-1\">also labeled</article>"
      "<!--id: PageSpeed-body-1,"
      " ElementTagDepth: 2,"
      " PreviousTagCount: 9,"
      " ContainedTagDepth: 2,"
      " ContainedTagRelativeDepth: 0,"
      " ContainedTagCount: 1,"
      " ContainedContentBytes: 12,"
      " ContainedNonBlankBytes: 11,"
      " ContainedNonAContentBytes: 12,"
      " div count: 1,"
      " parent role is content-->\n"
      "  <section id=\"PageSpeed-body-2\">this too\n"
      "    <aside id=\"PageSpeed-body-2-0\" data-mobile-role=\"marginal\">\n"
      "      and this, it differs.</aside>"
      "<!--id: PageSpeed-body-2-0,"
      " role: marginal,"
      " ElementTagDepth: 3,"
      " PreviousTagCount: 11,"
      " ContainedTagDepth: 3,"
      " ContainedTagRelativeDepth: 0,"
      " ContainedTagCount: 1,"
      " ContainedContentBytes: 21,"
      " ContainedNonBlankBytes: 18,"
      " ContainedNonAContentBytes: 21,"
      " div count: 1,"
      " parent role is content-->\n"
      "  </section>"
      "<!--id: PageSpeed-body-2,"
      " ElementTagDepth: 2,"
      " PreviousTagCount: 10,"
      " ContainedTagDepth: 3,"
      " ContainedTagRelativeDepth: 1,"
      " ContainedTagCount: 2,"
      " ContainedContentBytes: 29,"
      " ContainedNonBlankBytes: 25,"
      " ContainedNonAContentBytes: 29,"
      " div count: 1,"
      " section count: 1,"
      " parent role is content-->\n"
      "</div>"
      "<!--id: body,"
      " role: content,"
      " ElementTagDepth: 1,"
      " PreviousTagCount: 5,"
      " ContainedTagDepth: 4,"
      " ContainedTagRelativeDepth: 3,"
      " ContainedTagCount: 7,"
      " ContainedContentBytes: 57,"
      " ContainedNonBlankBytes: 52,"
      " ContainedNonAContentBytes: 57,"
      " body: 1,"
      " div count: 5,"
      " section count: 2-->\n"
      "<aside id=\"PageSpeed-5\" data-mobile-role=\"marginal\">Labeled</aside>"
      "<!--id: PageSpeed-5,"
      " role: marginal,"
      " ElementTagDepth: 1,"
      " PreviousTagCount: 12,"
      " ContainedTagDepth: 1,"
      " ContainedTagRelativeDepth: 0,"
      " ContainedTagCount: 1,"
      " ContainedContentBytes: 7,"
      " ContainedNonBlankBytes: 7,"
      " ContainedNonAContentBytes: 7,"
      " div count: 1-->\n"
      "<footer id=\"PageSpeed-6\" data-mobile-role=\"marginal\">labeled\n"
      "  <menu id=\"PageSpeed-6-0\" data-mobile-role=\"navigational\">\n"
      "    navvy</menu>"
      "<!--id: PageSpeed-6-0,"
      " role: navigational,"
      " ElementTagDepth: 2,"
      " PreviousTagCount: 14,"
      " ContainedTagDepth: 2,"
      " ContainedTagRelativeDepth: 0,"
      " ContainedTagCount: 1,"
      " ContainedContentBytes: 5,"
      " ContainedNonBlankBytes: 5,"
      " ContainedNonAContentBytes: 5,"
      " div count: 1-->\n"
      "</footer>"
      "<!--id: PageSpeed-6,"
      " role: marginal,"
      " ElementTagDepth: 1,"
      " PreviousTagCount: 13,"
      " ContainedTagDepth: 2,"
      " ContainedTagRelativeDepth: 1,"
      " ContainedTagCount: 2,"
      " ContainedContentBytes: 12,"
      " ContainedNonBlankBytes: 12,"
      " ContainedNonAContentBytes: 12,"
      " div count: 2-->\n"
      "<script type=\"text/javascript\">"
      "pagespeedHeaderIds=['PageSpeed-3'];\n"
      "pagespeedNavigationalIds="
      "['PageSpeed-1','PageSpeed-2','PageSpeed-6-0'];\n"
      "pagespeedContentIds=['body'];\n"
      "pagespeedMarginalIds="
      "['PageSpeed-body-2-0','PageSpeed-5','PageSpeed-6'];\n"
      "</script></body>";
  Parse("html5_tags_in_body", Unlabel(kLabeledHtml));
  GoogleString xbody = StrCat(doctype_string_, AddHtmlBody(kOutputHtml));
  RemoveRedundantDataFromOutputBuffer();
  EXPECT_STREQ(xbody, output_buffer_) << "html5_tags_in_body";
  EXPECT_EQ(1, pages_labeled_->Get());
  EXPECT_EQ(1, pages_role_added_->Get());
  EXPECT_EQ(3, navigational_roles_->Get());
  EXPECT_EQ(1, header_roles_->Get());
  EXPECT_EQ(1, content_roles_->Get());
  EXPECT_EQ(3, marginal_roles_->Get());
}

TEST_F(MobilizeLabelFilterTest, LargeUnlabeled) {
  StdioFileSystem filesystem;
  GoogleString original_filename =
      StrCat(GTestSrcDir(), kTestDataDir, kOriginal);
  GoogleString original_contents;
  ASSERT_TRUE(filesystem.ReadFile(
      original_filename.c_str(), &original_contents, message_handler()));
  GoogleString unlabeled_contents = Unlabel(original_contents);
  // Classify fully, compare against gold labeling.
  // Note that we don't necessarily match the labeling of the original!
  GoogleString labeled_filename =
      StrCat(GTestSrcDir(), kTestDataDir, kOriginalLabeled);
  GoogleString labeled_contents;
  ASSERT_TRUE(filesystem.ReadFile(
      labeled_filename.c_str(), &labeled_contents, message_handler()));
  ValidateExpected("unlabeled_adding_labels",
                   unlabeled_contents, labeled_contents);
  EXPECT_EQ(1, pages_labeled_->Get());
  EXPECT_EQ(1, pages_role_added_->Get());
  EXPECT_EQ(2, navigational_roles_->Get());
  EXPECT_EQ(2, header_roles_->Get());
  EXPECT_EQ(3, content_roles_->Get());
  EXPECT_EQ(1, marginal_roles_->Get());
  EXPECT_EQ(1, ambiguous_role_labels_->Get());
  EXPECT_EQ(12, divs_unlabeled_->Get());
}

}  // namespace

}  // namespace net_instaweb
