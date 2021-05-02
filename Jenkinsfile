pipeline {
    agent any

    stages {
        stage('Build') {
            steps {
                sh '''
                    cd src &&
                    ./cmp.sh
                '''
            }
        }
        stage('Deploy') {
            steps {
                sh '''
                    ssh -o NoHostAuthenticationForLocalhost=yes na@localhost
                '''
            }
        }
    }
}
