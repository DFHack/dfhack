GitHub workflows
================

We run our continuous integration (CI) validation and our release automation
via GitHub workflows. This allows us to merge PRs with confidence that they
won't catestrophically break DFHack functionality. GitHub workflows also allow
us to quickly produce stable release builds with fewer manual steps. Reducing
manual steps for releases is important since it is easy for a person to forget
a small but impactful step and therefore produce a bad release that causes
trouble for our users.

Background
----------

`GitHub workflows <https://docs.github.com/en/actions/writing-workflows>`_ run
on provisioned VMs in the cloud with stable environments that we specify. They
are free to use since DFHack is an open source project. They have proven to be
reliably available within a few seconds when our workflows are triggered. The
logic for the workflows is written in yaml, and the files that control our
workflows are stored in the :file:`.github/workflows/` directory in each of our
repos. Example: :source:`.github/workflows`.

Each workflow contains metadata that specifies:
- when it `triggers <https://docs.github.com/en/actions/writing-workflows/choosing-when-your-workflow-runs>`_
- what `base environment <https://docs.github.com/en/actions/using-github-hosted-runners/using-github-hosted-runners/about-github-hosted-runners>`_ it uses (OS, pre-installed dependencies, etc.)
- what additional dependencies should be installed (if any)
- custom business logic

Workflows run in the context of a single repo, but workflows defined in one
repo can inherit logic from workflows in other repos. All our common CI logic
is in the main DFHack/dfhack repo, but our submodules, like our ``scripts`` and
``df-structures`` repos, have CI workflows defined that inherit from the logic
in DFHack/dfhack. That way we can fix bugs and extend functionality in one
place and have it benefit the entire org tree.

Caches
~~~~~~

GitHub also provides 10GB per repository for `caches <https://docs.github.com/en/actions/writing-workflows/choosing-what-your-workflow-does/caching-dependencies-to-speed-up-workflows>`_.
We utilize the cache system to keep state between workflow runs, cache
downloads, and keep compiler output to speed up subsequent builds. Efficient
use of the cache system is a critical part of our workflow design. It allows us
to iterate on test failures in PRs in one minute instead of 20. It allows us to
put out an entire emergency release build in 5 minutes instead of 45. We have
tuned our build and test workflows to minimize spurious cache misses and keep
the fast path fast.

Caches are namespaced by key prefixes, and we have one key prefix per build
context. For example, release builds on gcc-10 are kept in one cache namespace,
whereas test builds on gcc-10 are kept separate. MSVC release and test builds
similarly have their own namespaces. Each cache has a maximum size that is
enforced by the business logic that writes the cache data.

In order to maintain consistency in a distributed environment, caches are
versioned. A workflow will read the latest version of the cache with its key
prefix, maybe modify the cache with new data, then write back a new version.
Caches that are not used for 2 weeks are purged from GitHub storage, but if a
repo goes over the 10GB limit, caches are deleted in LRU order until the repo
is under the storage limit again.

CI workflows
------------

Build
~~~~~

The Build workflow is the main CI workflow. It runs on every PR and push to a
branch. The ``build.yml`` file is essentially an orchestration layer for the
logic in several other .yml files:

- ``test.yml`` builds DFHack with the test suite enabled (but stonesense and
  windows pdb files disabled) and runs the test suite. It is optimized for
  speed and is intended to give PR authors quick feedback on their changes.
  The test suite is executed in a real running DF game on both Linux and
  Windows. The ``test`` job populates the ``test`` cache, which is used by many
  other workflows for non-distributed builds.
- ``package.yml`` builds DFHack as it would be released: test suite disabled
  but stonesense and windows pdb files enabled. The ``package`` job populates the ``release`` cache, which is used to build all distributed binaries.
- The ``docs`` target does a docs-only build of DFHack and reports any errors.
  Doc errors would show up in the ``test`` and ``package`` builds anyway, but
  the ``docs`` target runs very fast and can identify doc errors in less than
  1m.
- ``lint.yml`` runs the verification scripts in the ``ci`` directory. These
  scripts check for common errors in the codebase that are not caught by the
  compiler. The lint scripts are written in Python and shell script and are
  intended to be run quickly and catch common errors.

Check type sizes
~~~~~~~~~~~~~~~~

``check-type-sizes.yml`` is a df-structures-only workflow that checks for
changes in the sizes of types in the xml structures. It builds the
``xml-dump-type-sizes`` binary on both Linux and Windows for both the
structures in this PR and for the structures in the target merge branch. It
then runs the built binary on its native OS and compares the output. If any
type sizes have changed, the workflow generates a PR comment (via the
``comment-pr.yml`` workflow) with details.

.. _workflows-release-automation:

Release automation workflows
----------------------------

Watch DF Releases
~~~~~~~~~~~~~~~~~

This workflow runs every 8 minutes and checks the Steam metadata, the Itch
website, and the Bay 12 website for evidence of new releases. If a new release
is found, it generates an announcement in a private channel on the DFHack
Discord server.

Inside the ``watch-df-releases.yml`` workflow, there are separate jobs for
watching Steam branches and watching the websites. For the Steam watcher, it
takes configuration for:

- which branches to watch
- whether to kick off the Generate symbols workflow when a new release is found
- whether to autodeploy to Steam when the Generate symbols workflow completes

The workflow has protections against concurrent runs, so if you suspect a new
release is out, you can manually trigger the workflow to check. If the cron
trigger happens to run the workflow at the same time, the second run will be
paused while the first run completes.

Generate symbols
~~~~~~~~~~~~~~~~

This workflow can be triggered manually or by the Watch DF Releases workflow.
It downloads the specified DF version for the selected distribution platform(s)
and OS target(s), then updates the ``symbol-table`` entries in ``symbols.xml``.
If the distribution platform is Steam, it can also autodetect the DF version by
extracting the version string from the DF title screen data.

For Linux, it always builds DFHack -- just the core library (no plugins) -- and
generates symbols via the `devel/dump-offsets` and `devel/scan-vtables` scripts.

For Windows, we extract symbol data via static analysis, so the workflow only
builds DFHack if it needs to autodetect the DF version.

Once the symbols.xml file is updated, the workflow commits the changes to the
specified df-structures branch and updates the xml submodule ref in the
specified DFHack/dfhack branch. If a deploy Steam branch is specified, it also
launches the Deploy to Steam workflow.

Deploy to GitHub
~~~~~~~~~~~~~~~~

`github-release.yml <https://github.com/DFHack/dfhack/actions/workflows/github-release.yml>`_
can be triggered manually or automatically by creating a new release version
tag in git. It builds DFHack with the release configuration, packages the
aritifacts for GitHub, creates a new GitHub release, and uploads the packages
to the GitHub release page.

It uses text in :source:`.github/release_template.md` to generate the release
notes, and appends the changelog contents for the tagged version.

If you need to re-tag the release to fix a mistake, it will automatically run
again and replace the binaries attached to the GitHub release for the tagged
version. It will not overwrite the release notes, though, to preserve any edits
you may have made in the GitHub UI. If you *want* it to completely regenerate
the release notes, you can delete the release before you re-tag the version.

GitHub releases end up here: https://github.com/DFHack/dfhack/releases.

Deploy to Steam
~~~~~~~~~~~~~~~

`steam-deploy.yml <https://github.com/DFHack/dfhack/actions/workflows/steam-deploy.yml>`_
can be triggered manually or automatically by creating a new release version
tag in git. It builds DFHack with the release configuration, packages the
aritifacts for Steam, and uploads them to the specified Steam branch.

The workflow caches steamcmd to speed the deployment up by 30s or so.
Otherwise, steamcmd would have to be downloaded and updated every time the
workflow runs.

Steam releases end up here:
https://partner.steamgames.com/apps/builds/2346660. The "version" you
specified for the workflow is used as the "description" for the build.

Maintenance workflows
---------------------

Update submodules
~~~~~~~~~~~~~~~~~

`update-submodules.yml <https://github.com/DFHack/dfhack/actions/workflows/update-submodules.yml>`_
runs daily, or can be run manually as needed. It checks DFHack submodules for
new commits on the main branches and updates the submodule refs in the DFHack
develop branch.

You generally should not run this workflow for anything other than the develop
branch, as it will overwrite any changes you have made to the submodule refs in
other branches.

Clean up PR caches
~~~~~~~~~~~~~~~~~~

This workflow runs automatically whenever a PR is closed or merged. It removes
caches created for the PR so they don't take up quota.

Note that if you merge a PR before all the workflows have completed, the caches
may be created after this workflow runs. In that case, the caches will be
orphaned and will be purged by GitHub's cache eviction policy after 2 weeks.
