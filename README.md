# Operating Systems Project 22/23


## Installing / Getting started (WIP)

A quick introduction of the minimal setup you need to get the program running.

```shell
make default
make run
```

The default target cleans the directory, preps it with the release directories and ultimately compiles the project.
The run target executes the master process that's been compiled and is in the release subdirectory.

## Developing

### Built With
Built using C and makefile, no external libraries are used.

### Prerequisites
There are no real requirements other than a machine running macOS or Linux, supporting the execution of C in its ANSI standard (C89).

## Configuration (WIP)

Currently no configuration is needed. WIP to add a few cycle-depending variables such as the seconds for a day to pass etc.



## Tests (WIP)

Describe and show how to run the tests with code examples.
Explain what these tests test and why.

```shell
Give an example
```

## Git
### 1. Some Git rules

There are a set of rules to keep in mind:

- Perform work in a feature branch.

  _Why:_

  > Because this way all work is done in isolation on a dedicated branch rather than the main branch. It allows you to submit multiple pull requests without confusion. You can iterate without polluting the master branch with potentially unstable, unfinished code. [read more...](https://www.atlassian.com/git/tutorials/comparing-workflows#feature-branch-workflow)

- Never push into the `master` branch. Make a Pull Request.

  _Why:_

  > It notifies team members that you have completed a feature. It also enables easy peer-review of the code and dedicates a forum for discussing the proposed feature.

- Before making a Pull Request, make sure your feature branch builds successfully and passes all tests (including code style checks).

  _Why:_

  > You are about to add your code to a stable branch. If your feature-branch tests fail, there is a high chance that your destination branch build will fail too. Additionally, you need to apply code style check before making a Pull Request. It aids readability and reduces the chance of formatting fixes being mingled in with actual changes.

- Protect your `master` branch.

  _Why:_

  > It protects your production-ready branches from receiving unexpected and irreversible changes. read more... [GitHub](https://help.github.com/articles/about-protected-branches/), [Bitbucket](https://confluence.atlassian.com/bitbucketserver/using-branch-permissions-776639807.html) and [GitLab](https://docs.gitlab.com/ee/user/project/protected_branches.html)

### 2. Writing good commit messages

Having a good guideline for creating commits and sticking to it makes working with Git and collaborating with others a lot easier. Here are some rules of thumb ([source](https://chris.beams.io/posts/git-commit/#seven-rules)):

- Separate the subject from the body with a newline between the two.

  _Why:_

  > Git is smart enough to distinguish the first line of your commit message as your summary. In fact, if you try git shortlog, instead of git log, you will see a long list of commit messages, consisting of the id of the commit, and the summary only.

- Limit the subject line to 50 characters and Wrap the body at 72 characters.

  _Why:_

  > Commits should be as fine-grained and focused as possible, it is not the place to be verbose. [read more...](https://medium.com/@preslavrachev/what-s-with-the-50-72-rule-8a906f61f09c)

- Capitalize the subject line.
- Do not end the subject line with a period.
- Use [imperative mood](https://en.wikipedia.org/wiki/Imperative_mood) in the subject line.

  _Why:_

  > Rather than writing messages that say what a committer has done, it's better to consider these messages as the instruction for what is going to be done after the commit is applied on the repository. [read more...](https://news.ycombinator.com/item?id=2079612)

- Use the body to explain **what** and **why** as opposed to **how**.

## Style guide

Refer to [guidelines.md](guidelines.md)
