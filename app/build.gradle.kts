plugins {
    id("com.android.application")
    id("org.jetbrains.kotlin.android")
}

android {
    namespace = "com.neondistrict.afterglow"
    compileSdk = 34
    buildToolsVersion = "34.0.0"
    ndkVersion = "26.1.10909125"

    defaultConfig {
        applicationId = "com.neondistrict.afterglow"
        minSdk = 26
        targetSdk = 34
        versionCode = 1
        versionName = "0.0.1"

        ndk {
            abiFilters += "arm64-v8a"
        }

        externalNativeBuild {
            cmake {
                cppFlags += "-std=c++20"
                arguments += listOf("-DANDROID_STL=c++_shared")
            }
        }
    }

    buildTypes {
        debug {
            isDebuggable = true
            signingConfig = signingConfigs.getByName("debug")
        }
        release {
            isMinifyEnabled = false
            signingConfig = signingConfigs.getByName("debug")
        }
    }

    externalNativeBuild {
        cmake {
            path = file("src/main/cpp/CMakeLists.txt")
            version = "3.22.1"
        }
    }

    buildFeatures {
        prefab = true
    }

    compileOptions {
        sourceCompatibility = JavaVersion.VERSION_17
        targetCompatibility = JavaVersion.VERSION_17
    }

    kotlinOptions {
        jvmTarget = "17"
    }
}

afterEvaluate {
    tasks.matching { it.name == "mergeDebugAssets" }.configureEach {
        dependsOn("externalNativeBuildDebug")
    }
    tasks.matching { it.name == "mergeReleaseAssets" }.configureEach {
        dependsOn("externalNativeBuildRelease")
    }
}

dependencies {
    implementation("androidx.core:core-ktx:1.13.1")
    implementation("androidx.appcompat:appcompat:1.6.1")
    implementation("androidx.games:games-activity:2.0.2")
}
