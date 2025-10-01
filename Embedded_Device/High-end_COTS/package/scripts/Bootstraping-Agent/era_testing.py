import subprocess
import era_agent

bootstrapCommand = ['security_api_interface', 'bootstrap', 'Dummy']


def main():
    bootstrapStatus = subprocess.Popen(bootstrapCommand, stdout=subprocess.PIPE)
    if (bootstrapStatus.wait() != 0):
        print("Error during Bootstrapping")
        print(bootstrapStatus.stdout.read().decode('ascii'))
        return False
    else:
        print(bootstrapStatus.stdout.read().decode('ascii'))
        print("Bootstrapping successfull")

    era_agent.main()


if __name__ == "__main__":
    main()
