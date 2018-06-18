libilbc
=======

[![Build Status](https://travis-ci.org/TimothyGu/libilbc.svg)](https://travis-ci.org/TimothyGu/libilbc)

This is a packaging friendly copy of the iLBC codec from the WebRTC
project. It provides a base for distribution packages and can be used
as drop-in replacement for the non-free code from RFC 3591.

Contributing
------------

Only bug fixes and upstream merges are allowed. If you would like to fix
the source code that would benefit upstream as well, please consider sending
your patch to WebRTC first.

How Do I Merge Upstream Changes?
--------------------------------

It is very easy, as I have merged the WebRTC sources to create Git metadata
for file renames.

    git remote add upstream http://git.chromium.org/external/webrtc.git
    git fetch upstream
    git merge upstream/master

In most cases this will turn out fine. If there are some upstream additions,
you will have to remove them by hand.

**Always check `git status` before committing the merge** to make sure
nothing unneeded is added!!!
