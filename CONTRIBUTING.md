# Contributing

Contributions to librepsheet are always welcome! Before you hit the
pull request button there are a few things to consider. This project
runs at 100% code coverage (via lcov) and a clean Coverity
report. This might seem like a lot of additional work, but with a C
library meant for a wide variety of use cases it is important. Pull
requests that don't keep the coverage at 100% will not be accepted. If you
are adding new functions/functionality make sure to only add one
function or one functional group per pull request. This keeps the
review time to a minimum and the commits focused. The tests for this
project run on travis ci. A hook on the project will make sure the
tests are passing before pull requests are merged. If the tests are
not passing the pull request will be rejected. More info about your
setup as a developer can be found below.

## Developer Setup

If you wish to contribute to librepsheet you will need some additional
dependencies. The check, lcov, and doxygen libraries are required to
complete a full build of librepsheet. The test suite is built using
the check library. To run the tests simply run `make check`. The
documentation is created via doxygen. Generating new documentation
pages is done via `make doc`. Code coverage is done via lcov. To see a
code coverage report just run `script/coverage`. This will perform a
clean and full test run and generate the HTML coverage report. The
last step in `script/coverage` is to open the chromium browser on
Linux. If you don't have that installed or aren't running Linux this
step may fail.
