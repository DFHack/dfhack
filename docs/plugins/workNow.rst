workNow
=======
Don't allow dwarves to idle if any jobs are available.

When workNow is active, every time the game pauses, DF will make dwarves
perform any appropriate available jobs.  This includes when you one step
through the game using the pause menu.  Usage:

:workNow:       print workNow status
:workNow 0:     deactivate workNow
:workNow 1:     activate workNow (look for jobs on pause, and only then)
:workNow 2:     make dwarves look for jobs whenever a job completes
