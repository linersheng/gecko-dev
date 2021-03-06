package org.mozilla.gecko.tests;


public class testBookmarkKeyword extends AboutHomeTest {
    public void testBookmarkKeyword() {
        blockForGeckoReady();

        final String url = getAbsoluteUrl(StringHelper.ROBOCOP_BLANK_PAGE_01_URL);
        final String keyword = "testkeyword";

        // Add a bookmark, and update it to have a keyword.
        mDatabaseHelper.addOrUpdateMobileBookmark(StringHelper.ROBOCOP_BLANK_PAGE_01_TITLE, url);
        mDatabaseHelper.updateBookmark(url, StringHelper.ROBOCOP_BLANK_PAGE_01_TITLE, keyword);

        // Enter the keyword in the urlbar.
        inputAndLoadUrl(keyword);

        // Make sure the title of the page appeared.
        verifyUrlBarTitle(StringHelper.ROBOCOP_BLANK_PAGE_01_URL);

        // Delete the bookmark to clean up.
        mDatabaseHelper.deleteBookmark(url);
    }
}
