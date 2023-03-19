# Overview

This repo contains a Mosquitto 2.1 plugin that enforces **topic jail** for all connected clients - except for the **admin** client.

The default set of topics and structure are aligned with Microsoft Azure Device Provisioning Service (DPS) for zero touch provisioning via MQTT 3.1.1 protocol. While the topics can be overridden, at this time, the topics identified are precisely what DPS uses today. Any future changes to DPS are not part of this tool but given the configuration options, it may facilitate a smoother migration if topics are modified in the interface on DPS.

## Purpose

Topic jail prevents devices from either publishing or subscribing to topics that are tied exclusively to their connection. This prevents any cross talk amongst devices and ensures devices get just what they need to complete a zero touch provisioning message exchange.

## Configuration

In the 

## Buidling and Debugging

Currently the hosting GitHub repo has workflow actions defined on any push or pull request. This both builds and releases a `*.tar.gz` file containing just the binaries needed from Mosquitto and the Dynamic library for this plugin. 

After the release tagging, a Docker image is also built and currently published to Docker Hub at `cicorias/mosquitto-plugin-topic-jail`.

### Local Development

This repository contains a Devcontainer that builds and runs within Visual Studio Code or GitHub Codespaces an environment that permits code changes, debugging from within the IDE, and ability to interact with the host Git repository with the proper credentials for pull requests, etc.

Development is done using CMake and the corresponding CMake tools for Visual Studio Code. 

The fastest way to get up running and debugging is allow CMake tools to configure the project automaticaly.

#### Setup VS Code Settings

In the `./.vscode/settings.json` file for debugging THIS plugin -- ensure it looks similar to the following:

```json
{
    "cmake.buildDirectory": "${workspaceFolder}/build",
    "cmake.debugConfig": {
        "args": ["-c", "test.conf", "-v"],
        "cwd" : "${workspaceFolder}",
    }
}
```


 **TODO** Then choose the target -- which is this module from the CMake targets identified (there will be many) it should show as `./build/mosquitto_topic_jail.so` in the list.

At that point, a **Build** from the CMake tools and a **Debug** should just work.

## References
Mosquitto - 2.1 develop branch
CMake -
Device Provisioning Service Message formats

