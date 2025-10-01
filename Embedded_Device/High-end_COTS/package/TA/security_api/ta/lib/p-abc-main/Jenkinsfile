pipeline {
	agent any
	
	stages {
		stage('Build') {
			steps {
				sh '''
					rm -rf build
					mkdir build
					cd build
					cmake ..
					make
				'''
			}
		}

		stage('Test') {
			environment {
				CMOCKA_MESSAGE_OUTPUT = 'XML'
				CMOCKA_XML_FILE = '%g_cmtestresult.xml'
			}
			steps {
				sh '''
				    cd build
				    ctest
				'''
			}
		}
		
		stage('Report') {
			steps {
				junit 'build/**/*cmtestresult.xml'
			}
		}

		stage('Cleanup'){
			steps {
				sh '''
					rm -rf build
				'''
			}
		}
	}
}
