import util.compileSdk
import util.minSdk
import util.targetSdk

plugins {
    id("com.android.application")
    id("org.jetbrains.kotlin.android")
}

android {
    namespace = "com.alohamobile.alohacore.demo"
    compileSdk = project.compileSdk

    defaultConfig {
        applicationId = "com.alohamobile.alohacore.demo"
        minSdk = project.minSdk
        targetSdk = project.targetSdk
        versionCode = 1
        versionName = "1.0"

        testInstrumentationRunner = "androidx.test.runner.AndroidJUnitRunner"
    }

    buildTypes {
        release {
            isMinifyEnabled = false
            proguardFiles(getDefaultProguardFile("proguard-android-optimize.txt"), "proguard-rules.pro")
        }
    }

    // Disable resources compression to let AlohaCore access them via file descriptor.
    androidResources {
        noCompress.addAll(listOf("bin", "pak", "dat"))
    }
}

dependencies {
    implementation(libs.androidx.core)
    implementation(libs.androidx.appcompat)
    implementation(libs.material.design)
    implementation(libs.androidx.activity.ktx)
    implementation(libs.multidex)
    implementation(libs.guava)
    implementation(files("PATH_TO_ALOHA_CORE_AAR/aloha-bromium-localbuild.aar"))
}
