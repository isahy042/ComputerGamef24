The git checkin follows the following structure (roughly) as found:
https://stackoverflow.com/questions/61214752/how-to-maintain-a-build-directory-of-cmake-project-in-a-git-repository

Root                 
├───build/                  <---- Do NOT check in anything from 'build'.
│   CMakeLists.txt          <---- Check in your top-level CMake source file.
├───src/                    <---- Check in all your code and other CMake files.
│       CMakeLists.txt
│       MyClass.cpp
│       main.cpp