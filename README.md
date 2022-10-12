<img src="https://informatica.i-learn.unito.it/pluginfile.php/123954/user/icon/lambda/f1?rev=291858" alt="Logo of the project" align="left">
<img src="https://share.cleanshot.com/8YNTkS/download" alt="Logo 2 of the project" align="right">

# Operating Systems Project 22/23


## Installing / Getting started (WIP)

A quick introduction of the minimal setup you need to get a hello world up &
running.

```shell
commands here
```

Here you should say what actually happens when you execute the code above.

## Developing

### Built With (WIP)
List main libraries, frameworks used including versions (React, Angular etc...)

### Prerequisites (WIP)
What is needed to set up the dev environment. For instance, global dependencies or any other tools. include download links.


We can maybe use [SemVer](http://semver.org/) for versioning. For the versions available, see the [link to tags on this repository](/tags).


## Configuration (WIP)

Here you should write what are all of the configurations a user can enter when using the project.



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

- Update your local branch and do an interactive rebase before pushing your feature and making a Pull Request.

  _Why:_

  > Rebasing will merge in the requested branch (`master`) and apply the commits that you have made locally to the top of the history without creating a merge commit (assuming there were no conflicts). Resulting in a nice and clean history. [read more ...](https://www.atlassian.com/git/tutorials/merging-vs-rebasing)

- Resolve potential conflicts while rebasing and before making a Pull Request.
- Delete local and remote feature branches after merging.

  _Why:_

  > It will clutter up your list of branches with dead branches. It ensures you only ever merge the branch back into (`master`) once. Feature branches should only exist while the work is still in progress.

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

## Style guide (WIP)

Refer to [guidelines.md](guidelines.md)
