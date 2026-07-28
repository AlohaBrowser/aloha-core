@file:Suppress("unused", "UnusedReceiverParameter")

package util

import org.gradle.api.Project

private const val MIN_SDK_VERSION = 29
private const val TARGET_SDK_VERSION = 36
private const val COMPILE_SDK_VERSION = 36

val Project.minSdk: Int
    get() = MIN_SDK_VERSION

val Project.targetSdk: Int
    get() = TARGET_SDK_VERSION

val Project.compileSdk: Int
    get() = COMPILE_SDK_VERSION
