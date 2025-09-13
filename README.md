# Micromouse 2023-24 🐁

A fully autonomous maze-solving not-quite-mouse-sized robot. 

## Folders

```
.
├── LICENSE	     # License (woohoo)
├── README.md    # This file
├── cad          # 3D-models and designs
├── pcb          # Hardware-related designs and component docs
└── software     # Soft/firmware for the robot
```

## Naming Guidelines
* For branches, use kebab case (kebab-case). The name should align with the Git issue/new feature name.
* For files and folders, use standard C/C++ naming conventions (snake_case).
* For variables and functions, use C++/Arduino naming conventions (camelCase). For constants, use all caps (SNAKE_CASE).

## Commit Guidelines
* When working on a new feature, if there is not already a Git issue created, create a new Git issue describing the proposed changes. Also create a new branch and associated pull request draft.
* Assign the issue and pull request to whoever is working on the features.
* Once the code is ready for review, update the pull request accordingly.
* Once the code has been tested and is working on the robot, merge the changes into main and close the pull request.