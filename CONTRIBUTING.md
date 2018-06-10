# CONTRIBUTING

Thanks for considering contributing to Onion.

Following these guidelines helps to communicate that you respect the time of the
developers managing and developing this open source project. In return, they
should reciprocate that respect in addressing your issue, assessing changes, and
helping you finalize your pull requests.

Onion is an open source project and we love to receive contributions from our
community — you! There are many ways to contribute, from writing tutorials or
blog posts, improving the documentation, submitting bug reports and feature
requests or writing code which can be incorporated into onion.

## How to report a bug

If the bug has security implications, please consider 90 [days responsible
disclosure](https://en.wikipedia.org/wiki/Responsible_disclosure)

Any security issues should be submitted directly to dmoreno@coralbits.com.

For other bugs open an Issue at the [Issue
tracker](https://github.com/davidmoreno/onion/issues). Remember to always
include:

1. Version of onion (or master)
2. Example source code that triggers the bug
3. Steps to trigger the bug
4. Current result of execution
5. Expected result

Extra points if the test code can be automated and added to the test suite.

## How to suggest features and changes

Add an issue with the `feature request` tag. Include the following information:

1. Feature description
2. Example use case
3. How important do you feel it is
4. Impact you think may have on onion

## Support request

If you need support for running your code, please write to the [mailing
list](https://groups.google.com/a/coralbits.com/group/onion-dev/topics) or open
a [stack overflow question](https://stackoverflow.com/search?q=libonion).


## Contributng code to onion

### Ground rules

Responsibilities

 * Ensure cross-platform compatibility for every change that's accepted on
   various GNU/Linux distributions and if possible on MacOS.

 * Ensure that code that goes into core meets all requirements in this
   checklist at the end of this document.
 * Create issues for any major changes and enhancements that you wish to make.
   Discuss things transparently and get community feedback.
 * Keep feature branches as small as possible, preferably one new feature
   per branch.
 * Be welcoming to newcomers and encourage diverse new contributors from all
   backgrounds.

### Getting Started

Unsure where to begin contributing to Onion? You can start by looking through
these beginner and help-wanted issues:

 * `Beginner` issues - issues which should only require a few lines of code, and
   a test or two.
 * `Help wanted` issues - issues which should be a bit more involved than
   beginner issues.

Both issue lists are sorted by total number of comments. While not perfect,
number of comments is a reasonable proxy for impact a given change will have.

### Adding your code to onion

To contribute code to onion follow these steps:

1. If its a big change or may be interesting to other onion users/developers,
   create an issue describing what you want to achieve and state you are working
   on it.
2. Create your own fork of the code from master
3. Do the changes in your fork
4. Ensure that it can be applied to master cleanly. Normally a
   `git fetch; git merge master` at your branch is enough.
5. Execute the `make indent` to ensure standard indentation of code.
6. Create the pull request
7. Be attentive for questions or suggested changes on the pull request or in
   the issue.

### Contributor License Agreement

Onion is distributed under GPL2 and Apache2 licenses.

If you submit code to onion you agree that this code has been developed by you,
or have permission over it, and agree to license it under the same terms.


---
This document is based on https://github.com/nayafia/contributing-template/blob/master/CONTRIBUTING-template.md
