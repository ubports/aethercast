pipeline {
  agent any
  stages {
    stage('Preparation') {
      steps {
        dir(path: 'source') {
          git 'https://github.com/ubports/aethercast.git'
        }
        
      }
    }
    stage('Build source') {
      steps {
        sh 'rm -f ./* || true'
        sh '''cd source
export GIT_COMMIT=$(git rev-parse HEAD)
export GIT_BRANCH=$(git rev-parse --abbrev-ref HEAD)
cd ..
/usr/bin/generate-git-snapshot
'''
        stash(name: 'source', includes: '*.gz,*.bz2,*.xz,*.deb,*.dsc,*.changes,*.buildinfo,lintian.txt')
      }
    }
    stage('Build binary - armhf') {
      steps {
        node(label: 'xenial-arm64') {
          unstash 'source'
          sh '''export architecture="armhf"
export distribution="xenial"
export REPOS="xenial"
export BUILD_ONLY=true
export DEB_BUILD_OPTIONS="parallel=$(nproc) nocheck"
/usr/bin/generate-reprepro-codename "${REPOS}"
/usr/bin/build-and-provide-package'''
          stash(includes: '*.gz,*.bz2,*.xz,*.deb,*.dsc,*.changes,*.buildinfo,lintian.txt', name: 'build')
          cleanWs(cleanWhenAborted: true, cleanWhenFailure: true, cleanWhenNotBuilt: true, cleanWhenSuccess: true, cleanWhenUnstable: true, deleteDirs: true)
        }
        
      }
    }
    stage('Results') {
      steps {
        unstash 'build'
        archiveArtifacts(artifacts: '*.gz,*.bz2,*.xz,*.deb,*.dsc,*.changes,*.buildinfo', fingerprint: true, onlyIfSuccessful: true)
        sh '''export architecture="armhf"
export REPOS="xenial"
mkdir -p binaries

for suffix in gz bz2 xz deb dsc changes ; do
  mv *.${suffix} binaries/ || true
done

export BASE_PATH="binaries/"
export PROVIDE_ONLY=true
/usr/bin/build-and-provide-package'''
      }
    }
    stage('Cleanup') {
      steps {
        cleanWs(cleanWhenAborted: true, cleanWhenFailure: true, cleanWhenNotBuilt: true, cleanWhenSuccess: true, cleanWhenUnstable: true, deleteDirs: true)
      }
    }
  }
}
