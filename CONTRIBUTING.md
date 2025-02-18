[//]: # (
Copyright Quadrivium LLC
All Rights Reserved
SPDX-License-Identifier: Apache-2.0
)

# Contributing to Kagome

All types of contributions are encouraged and valued. See the [Table of Contents](#table-of-contents) for different ways to help and details about how this project handles them. Please make sure to read the relevant section before making your contribution. It will make it a lot easier for us maintainers and smooth out the experience for all involved. The community looks forward to your contributions. 🎉

## Table of Contents

- [Code of Conduct](#code-of-conduct)
- [I Have a Question](#i-have-a-question)
- [Contributing](#contributing)
    - [Styleguides](#styleguides)
        - [Documentation](#documentation)
        - [Commit Messages](#commit-messages)
        - [Pull Requests](#pull-requests)
    - [Reporting Bugs](#reporting-bugs)

## Code of Conduct

This project and everyone participating in it is governed by the [Code of Conduct](CODE_OF_CONDUCT.md). By participating, you are expected to uphold this code. 

## I Have a Question

Before you ask a question, it is best to search for existing [Issues](https://github.com/qdrvm/kagome/issues) that might help you. In case you have found a suitable issue and still need clarification, you can write your question in this issue. It is also advisable to search the internet for answers first.

If you then still feel the need to ask a question and need clarification, we recommend the following:

- Open an [Issue](https://github.com/qdrvm/kagome/issues/new).
- Provide as much context as you can about what you're running into.
- Provide version of KAGOME and platform (OS, compiler, etc.) you are using if applicable.

We will then take care of the issue as soon as possible. Be patient, as we are all volunteers and have other obligations as well.

## Contributing

### Styleguides

1. KAGOME uses C++20 as target language.
2. We are following Google C++ Style Guide. Please refer to [Google C++ Style Guide](https://google.github.io/styleguide/cppguide.html) for more details.
2. Every new code should be documented. Ensure that your code is clear and well-commented.
3. New code cannot be merged into master if it contains unresolved TODOs without a link to the opened issue, where this TODO should be resolved.
4. New code should be covered by unit tests using gtest/gmock. Make sure to write and run tests to ensure your code works as expected.
5. Use `clang-format` to maintain the code style.
7. Open PR with base branch = `master`, fix CI and follow the guide in PR template.

#### Documentation

Good documentation is crucial for understanding the purpose and usage of different parts of the code. Here are some guidelines on how to document your code:

1. **File-Level Comments**: At the top of each file, you should have comments that describe the content of the file, its purpose, and how it interacts with the rest of the codebase. This is especially important for header files (`.hpp`), as they define the interface of your code.

```cpp
// File: example.hpp
// This file provides an example interface for ...
```

2. **Class Comments**: Each class should have an associated comment that describes what it is and what it does. This should be placed immediately before the class definition in the header file (.hpp).

```cpp
// @class Example
// @brief The Example class is used to ...
// The Example class is doing that as follows: ...
class Example {
    ...
};
```

3. **Method Comments**: Each method should have a comment describing what it does. This should be placed immediately before the method declaration in the header file (.hpp). For non-trivial methods, also document its parameters and return value. For trivial methods, these can be omitted.

```cpp
// @brief This method is used to ...
// Parameters:
// @param param1 is used to ...
// @param param2 is used to ...
// @return Returns value that contains ...
Value exampleMethod(int param1, std::string param2);
```

4. **Method Body Comments**: Inside method bodies in the source files (.cpp), use comments to explain complex or non-obvious parts of the code. Avoid obvious comments that don't add any new information.

```cpp
void Example::exampleMethod(int param1, std::string param2) {
  // Here we're doing a complex operation
  ...
  // Here we're doing another complex operation
  ...
}
```

5. **References**: If you're using external libraries, APIs, or resources, provide references to them in your comments. Considering that we are developing a Polkadot Host, it is important to reference the Polkadot Wiki, Polkadot Spec, Polkadot-SDK documentation, or other relevant resources when necessary. This helps other developers understand where the code comes from and how it works.

```cpp
// This code is based on the algorithm described in the paper "Title of the Paper" by Author et al.
```

```cpp
/**
* Make ancestry merke proof for GrandpaJustification.
* https://github.com/paritytech/polkadot-sdk/blob/4842faf65d3628586d304fbcb6cb19b17b4a629c/substrate/client/consensus/grandpa/src/justification.rs#L64-L126
*/
inline outcome::result<void> makeAncestry(
  GrandpaJustification &justification,
  const blockchain::BlockTree &block_tree) {
...
}
```

Remember, the goal of comments is to help other developers (and your future self) understand the code. They should be clear, concise, and informative.

#### Commit Messages

Here's a general guideline for writing good commit messages:  
1. **Use the Imperative Mood**: Start your commit messages in the imperative mood, "Fix bug" and not "Fixed bug" or "Fixes bug". This convention matches up with commit messages generated by commands like git merge and git revert.  
2. **First Line is a Summary**: The first line of the commit message should be a brief summary of the changes, followed by a blank line, and then a detailed description (if needed). The first line should be limited to 50 characters and written in the imperative mood.  
3. **Explain the Why, not the What**: The code diff already shows what changes you made, so use the commit message to explain why you made those changes.  
Here's an example of a good commit message:
```
Add error handling for invalid user input

- Add try/catch block in user input processing function
- Return meaningful error messages to the user
- This change is necessary to improve user experience and handle potential errors
- Related to issue #123
```

#### Pull Requests

Pull Requests (PRs) are a vital part of any collaborative project. They allow developers to propose changes, get feedback, and merge their code into the main codebase. Here are some good practices for opening PRs:

1. **Branch Naming**: Name your branch something descriptive and relevant to the changes you're proposing. This makes it easier for others to understand what your PR is about just by looking at the branch name. It's a good practice to prefix your branch name with a category such as `feature/`, `bug/`, `test/`, `doc/`, `refactor/`, or `fix/`. For example, if you're adding a new feature related to authentication, you might name your branch `feature/authentication`.

2. **PR Title**: Like the branch name, the PR title should be descriptive of the changes. If your PR fixes a bug or adds a feature, state that in the title.

3. **PR Description**: The description should provide a detailed explanation of the changes you've made. Explain why you made the changes, how you made them, and any other relevant information. This helps reviewers understand your thought process and the context behind the PR.

4. **Linking Issues**: If your PR corresponds to an existing issue, mention this in the PR's description. You can do this by typing `#` followed by the issue number. This creates a link between the PR and the issue, providing further context and helping track the progress of tasks.

In addition, GitHub recognizes certain keywords to close an issue automatically once the PR is merged. These keywords are: `close`, `closes`, `closed`, `fix`, `fixes`, `fixed`, `resolve`, `resolves`, `resolved`. If your PR completely resolves the issue, you can include one of these keywords before the issue number. For example:

```markdown
Closes #123
```

This will automatically close the issue #123 when the PR is merged into the main branch. This practice helps to automate the issue-tracking process and ensures that no issue is accidentally left open after its corresponding changes are merged.

5. **Small, Focused PRs**: Try to keep your PRs small and focused on a single task, feature, or bug fix. This makes the PR easier to review and understand. Large, complex PRs can be difficult to review and may delay the merging process.

6. **Review Your Own PR**: Before requesting reviews from others, review your own PR. This can help you catch errors, improve the quality of your code, and make the review process smoother for everyone.

7. **Approval by Maintainers**: Before a PR can be merged, it should be approved by at least two maintainers. This ensures that the changes have been thoroughly reviewed and are in line with the project's standards and goals. When assigning reviewers, try to choose maintainers who are most familiar with the code you've changed. Alternatively, you can use GitHub's reviewer suggestions, but it's a good practice to try to assign maintainers who don't already have a large number of PRs assigned for their review. This helps distribute the review workload evenly among the team.

8. **Merging Approach**: In our project, all branches should be merged using the "Squash and Merge" approach. This means that all commits in the branch will be squashed into a single commit when merging into the main branch. This approach helps to keep the commit history of the main branch clean and understandable. It's important to ensure that your branch has a meaningful and comprehensive commit message that reflects the changes made in the entire branch.

9. **Merge Commit Title**: When merging your branch, ensure that the merge commit has a meaningful title. This title should summarize the changes made in the branch and provide context about what the merge adds to the main branch. This is especially important when using the "Squash and Merge" approach, as all the changes from the branch will be represented by this single commit in the main branch's history. A good practice is to use the PR title or a summary of it as the merge commit title.

Remember, the goal of a PR is not just to merge code into the main codebase. It's also an opportunity for team collaboration, code review, and learning. Make your PRs clear, concise, and informative to make the most of this process.

### Reporting Bugs

If you've found a bug in KAGOME, your contribution towards fixing it is greatly appreciated. Here are some guidelines to follow when reporting bugs:

1. **Check Existing Issues**: Before reporting a bug, please check the existing [Issues](https://github.com/qdrvm/kagome/issues) to see if it has already been reported. If it has, you can add any additional information you have to the existing issue.

2. **Create a New Issue**: If the bug hasn't been reported yet, create a new issue in the [Issues](https://github.com/qdrvm/kagome/issues/new) section. Use a clear and descriptive title for the issue to help others understand what the bug is about.

3. **Describe the Bug**: In the issue description, provide a detailed explanation of the bug. Include information about what you expected to happen and what actually happened. If possible, provide steps to reproduce the bug. This will help others to understand and fix the bug.

4. **Include Error Messages and Debugging Artifacts**: If there are any error messages, stack traces, or core dumps related to the bug, include them in the issue. These can provide valuable information for diagnosing the problem.

5. **Provide System Information**: Include information about your system, such as the operating system, the version of KAGOME you're using, and any other relevant software versions. This can help identify if the bug is specific to certain environments.

6. **Use Labels**: If possible, use labels to categorize the bug. This can help maintainers and other contributors to find and prioritize the bug.

7. **Be Respectful and Constructive**: Remember that KAGOME is maintained by volunteers who are donating their time to the project. When reporting bugs, be respectful and constructive, and remember to follow the project's [Code of Conduct](#code-of-conduct).

Remember, the goal of reporting bugs is not just to get them fixed, but also to contribute to the project and the community. Your bug reports help to make KAGOME better for everyone. Thank you for your contributions!

