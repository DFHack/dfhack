Release process
===============

This page details the process we follow for beta and stable releases.

For documentation on the related GitHub workflows, see
`workflows-release-automation`.

Beta release
------------

This process pushes a pre-release build to GitHub and Steam. It is intended to
be lower-toil than the stable release process and allows us to facilitate
frequent public testing and feedback without compromising the stability of our
"stable" releases.

1. Run the `Update submodules <https://github.com/DFHack/dfhack/actions/workflows/update-submodules.yml>`_ GitHub action on the ``develop`` branch to ensure that all submodules are up to date.

2. Update version strings in :source:`CMakeLists.txt` as appropriate. Set ``DFHACK_RELEASE`` to the *next* stable release version with an "rc#" suffix. For example, if the last stable release was "r1" then set the string to "r2rc1". If we do a second beta release before the final stable "r2" then the string would be "r2rc2".

    - Ensure the ``DFHACK_PRERELEASE`` flag is set to ``TRUE``.
    - Commit and push to ``develop``
    - Set ``RELEASE`` in your environment for the commands below (e.g. ``RELEASE=51.07-r2rc1`` for bash)

3. Tag ``develop`` (no need to tag the submodules) and push: ``git tag -a $RELEASE -m "Bump to $RELEASE"; git push --tags origin``

    - This will automatically trigger `Deploy to Steam <https://github.com/DFHack/dfhack/actions/workflows/steam-deploy.yml>`_ and `Deploy to GitHub <https://github.com/DFHack/dfhack/actions/workflows/github-release.yml>`_ build workflows.

4. Write release notes highlights and any requests for feedback in the `draft GitHub release <https://github.com/DFHack/dfhack/releases>`_ and publish the draft.

5. Associate release notes with the build on Steam

    - Go to the `announcement creation page <https://steamcommunity.com/games/2346660/partnerevents/create>`_
    - Select "A game update"
    - Select "Small update / Patch notes"
    - Set the "Event title" to the release version (e.g. "DFHack 51.07-r2rc1")
    - Set the "Subtitle" to "DFHack pre-release (beta channel)"
    - Mention release highlights in the "Summary" field
    - Transcode release notes from the GitHub release into the "Event description" field
        - patch notes must be in BBcode; see `converting-markdown-to-bbcode` below for how to convert our release notes to BBcode
    - Click "Link to build" and select the staging branch (be sure the build has been deployed to the branch first. By the time you're done with the patch notes the build will likely be ready for you)
    - Go to the Artwork tab, select "Previously uploaded images", and search for and double-click on dfhack_logo.png. Click "Upload" (even though it has already been uploaded).
    - Switch to the "Publish" tab and publish!
    - `Promote <https://partner.steamgames.com/apps/builds/2346660>`_ the build to the "beta" branch (and the "testing" branch if it's newer than what is on the "testing" branch)

6. Monitor for beta channel subscriber feedback on the Steam `community page <https://steamcommunity.com/app/2346660/eventcomments/>`_

7. *Maybe* also post to Reddit and other announcement channels if we feel like we need to recruit more beta testers into the pool, but we should avoid posting so often that it is annoying for those who don't use Steam or just want announcements for stable releases.

Stable release
--------------

This process creates a stable DFHack release meant for widespread distribution.
Stable releases come in two forms: straight from ``develop`` or from a point
release branch.

During "normal" times, we will test out new features in beta releases until we
reach a point of stability. Then, after the ``develop`` branch is feature frozen
while we polish and fix bugs, we tag a release directly from ``develop``
``HEAD``.

However, if we have already started committing beta features to ``develop`` and
it becomes necessary to put out a bugfix release for a problem in an
already-released stable release, then we will create a new branch from the
stable tag, cherry-pick fixes from ``develop`` onto that branch, and spin a
release from there. After the point release is published, we'll merge the
branch back into ``develop`` and remove the release branch to clean up.

1. Triage remaining issues/PRs in the `release project <https://github.com/orgs/dfhack/projects>`_

    - Don't feel pressure to merge anything risky just before a stable release. That's what beta releases are for.

2. In your local clone of the ``DFHack/develop`` branch, make sure your checkout and all submodules (listed in :source:`.gitmodules`) are up to date with their latest public commits and have no uncommitted/unpushed local changes.

3. Ensure that CI has not failed unexpectedly on the latest online changes:

    - https://github.com/DFHack/dfhack/commits/develop
    - https://github.com/DFHack/scripts/commits/master
    - https://github.com/DFHack/df-structures/commits/master

4. Update version strings in :source:`CMakeLists.txt` as appropriate

    - Ensure the ``DFHACK_PRERELEASE`` flag is set to ``FALSE``.
    - Set ``RELEASE`` in your environment for the commands below (e.g. ``RELEASE=51.07-r1``)

5. Replace "Future" with the version number and clean up changelog entries; add new "Future" section (with headers pre-populated from the template at the top of the file):

    - ``docs/changelog.txt``
    - ``scripts/changelog.txt``
    - ``library/xml/changelog.txt``
    - ``plugins/stonesense/docs/changelog.txt``

6. Do a top-level build to ensure the docs build cleanly

7. Commit/push changes to submodules and tag (``git tag -a $RELEASE -m "Bump to $RELEASE"; git push --tags origin master``)

    - ``scripts``
    - ``library/xml``
    - ``plugins/stonesense``

8. Commit and push changes to ``develop``

    - Ensure that any updates you pushed to submodules are tracked in the commit to ``DFHack/develop``

9. Tag ``dfhack``: ``git tag -a $RELEASE -m "Bump to $RELEASE"; git push --tags origin develop``

    - This will automatically trigger a `Deploy to Steam <https://github.com/DFHack/dfhack/actions/workflows/steam-deploy.yml>`_ GitHub action to the "staging" Steam branch and a `Deploy to GitHub <https://github.com/DFHack/dfhack/actions/workflows/github-release.yml>`_ GitHub action to create a draft `release <https://github.com/DFHack/dfhack/releases>`_ from a template and attach the built artifacts.

10. Switch to the Steam ``staging`` release channel in the Steam client (password: ``stagingstagingstaging``) and download/test the update.

    - Ensure DFHack starts DF when run from the Steam client
    - Ensure the DFHack version string is accurate on the title page (should just be the release number, e.g. ``DFHack 51.07-r1``, with no git hash or warnings)
    - Run `devel/check-release`
    - If something goes wrong with this step, fix it, delete the tag (both from `GitHub <https://github.com/DFHack/dfhack/tags>`_ and locally (``git tag -d $RELEASE``)), re-tag, re-push, and re-test. Note that you do *not* need to remove the GitHub draft release -- the existing one will just get updated with the new tag and binaries. You *can* remove the draft release, though, if you want the release notes to get regenerated.

11. Prep release on GitHub

    - Go to the draft `release <https://github.com/DFHack/dfhack/releases>`_ on GitHub
    - Add announcements, highlights (with demo videos), etc. to the description

12. Push develop to master (``git push origin develop:master``)

    - This will start the documentation build process and update the published "stable" docs
    - Note that if this is a -r1 release, you won't be able to complete this step until a classic build is available on the Bay 12 website so the DFHack Test workflow can pass, which is a prerequisite for being able to push to ``master``.

13. Post release notes on Steam

    - Go to the `announcement creation page <https://steamcommunity.com/games/2346660/partnerevents/create>`_
    - Select "A game update"
    - Select "Regular update"
    - Set the "Event title" to the release version (e.g. "DFHack 51.07-r1")
    - Set the "Subtitle" to "DFHack stable release"
    - Add list of highlights (and maybe announcements, if significant) to the "Summary" field
    - Upload screenshots and demo videos via the button at the bottom of the "Previously uploaded videos" area
    - Add release notes to the "Event description" field (must be in BBcode; see `converting-markdown-to-bbcode` below for how to convert our release notes to BBcode)
        - Drag uploaded images/videos into their appropriate places in the announcement text (replace the GitHub URL tags, which won't work from Steam)
        - If the generated release notes exceed the announcement length limits, add a link to the GitHub release page at the bottom of the announcement instead
    - Click "Link to build" and select the staging branch (be sure the build has been deployed to the branch first. by the time you're done with the patch notes the build will likely be ready for you)
        - the release notes will travel with the build when we promote it to other branches
    - Go to the Artwork tab, select "Previously uploaded images", and search for and double-click on STABLEannouncement6.png. Click "Upload" (even though it has already been uploaded).
    - Switch to the "Publish" tab and publish!

14. Go to the `Steam builds page <https://partner.steamgames.com/apps/builds/2346660>`_ and promote the build to the "default" branch

    - For the build that you just pushed to "staging", click the "-- Select an app branch --" drop-down and select "default"
    - Click on "Preview Change"
    - Commit the change (you may need to verify with 2FA)
    - If the release is newer than what's on the ``beta`` and/or ``testing`` branches, set it live on those branches as well

15. Publish the prepped GitHub release

16. Send out release announcements

    - Announce new version in r/dwarffortress. Example: https://www.reddit.com/r/dwarffortress/comments/1i3l5xl/dfhack_5015r2_released_highlights_stonesense/
        - Create the post in the Reddit web interface; the mobile app is extremely painful to use for posting
        - Do an "Images & Video" post, sample title: "DFHack 51.07-r1 released! Hilights: Open legends mode directly from an active fort, Dig through warm or damp tiles without interruption, Unlink buildings from levers"
        - Add the animated gifs to the post (with appropriate captions naming the relevant tool and what is being demonstrated)
        - Add the "DFHack Official" flair to the post. If you're not a r/dwarffortress mod, ask Myk to do this after posting.
        - After posting, add each section of the release notes as its own comment, splitting out individual announcements and highlights. This gives people the opportunity to respond directly to the portion of the release notes that interests them; it also helps us avoid size limits for comments. You can include a single still shot (.png file) per comment, but you have to switch to "Fancy Pants Editor" to do it. You can only switch editors once, or the image will get messed up (that is, the image will turn into a hyperlink to an image). Suggested procedure is to prepare the comment in markdown, switch to Fancy Pants Editor, and add images just before submitting the comment.
    - Announce new version in forum thread. Example: http://www.bay12forums.com/smf/index.php?topic=164123.msg8567134#msg8567134
        - Update latest version text and link in `first post <http://www.bay12forums.com/smf/index.php?topic=164123.msg7453045#msg7453045>`_ (if you are not Lethosor, ping Lethosor for this)
    - Announce in `#announcements <https://discord.com/channels/793331351645323264/807444616731295755>`_ on DFHack Discord
    - Announce in `#mod-releases <https://discord.com/channels/329272032778780672/1066180550114680853>`_ on Kitfox Discord
        - Change the name of the release thread on Kitfox Discord to match the release version (if you are not Myk, ping Myk for this)

17. Monitor all announcement channels for feedback and respond to questions/complaints

18. Create a `project <https://github.com/orgs/dfhack/projects>`_ on GitHub in the DFHack org for the next release

    - Open the `project template <https://github.com/orgs/DFHack/projects/52>`_
    - Click "Use this template"
    - Name the project according to the version, e.g. "51.07-r2" and click "Use template"
    - In the new project, select settings and set the visibility to Public
    - Move any remaining To Do or In Progress items from last release project to next release project
    - Close project for last release

19. If this is a -r2 release or later, go to https://readthedocs.org/projects/dfhack/versions/ and "Edit" previous DFHack releases for the same DF version and mark them "Hidden" (keep the "Active" flag set) so they no longer appear on the docs version selector.

.. _converting-markdown-to-bbcode:

Converting Markdown to BBcode
-----------------------------

Hopefully we can `automate <https://github.com/DFHack/dfhack/issues/3268>`_ this in the future, but for now, here is the procedure:

1. Get the markdown that you want to convert into some field on GitHub (can be a temporary text field that you then preview without saving)

2. View the rendered release notes in your browser (these instructions are for Chrome, but other browsers probably have similar capabilities)

3. Right click on the rendered text and inspect the DOM

4. Copy the HTML element that contains the release notes

5. Click on the "Import HTML" button on the Steam announcement form; paste in the HTML and click "Overwrite"

6. Copy the generated BBCode out from the description field and into a text editor

7. Fix it up:

    - Remove the "How do I download DFHack?" section -- people on Steam don't need it
    - Some ``<h3>`` elements aren't converted properly and need to be rewritten with square brackets
    - Any monospaced text gets HTML tags instead of BBCode ``[code]`` tags, but you can't use them either since they force newlines. ``[tt]`` isn't supported. Any ``<code>`` tags just need to be removed entirely.
    - Any ``<details>`` and ``<summary>`` tags need to be removed

8. Copy it all back into the description field for the announcement

9. Click on "Preview event" to double check that it renders sanely

10. You're done.
