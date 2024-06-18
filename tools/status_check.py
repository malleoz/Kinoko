# Validate that the framecounts in STATUS.md are up-to-date. This is done by trying to run the entire test case and
# asserting that the sync framecount is the highest it currently can be. This helps us capture when changes focused
# on one test case actually benefit a different test case.

from generate_tests import generate_tests
import json
import subprocess
import sys

STATUS_TEST_CASE_FILENAME = 'statusTestCases.json'

class TestCase:
    def __init__(self, line):
        # Get the test case name in between the square brackets
        self.name = line[line.find('[')+2:line.find(']')-1]

        # Get the frame counts in between the pipes
        frameCounts = line.split('|')[1].split('/')
        self.syncFrame = int(frameCounts[0])
        self.totalFrames = int(frameCounts[1])

    # While normally "targetFrame" would just be the lower bound, we want to run the upper bound
    # and see if it produces a later desync than we currently are tracking in STATUS.md
    def toJson(self):
        return {
            "rkgPath": "samples/" + self.name + ".rkg",
            "krkgPath": "samples/" + self.name + ".krkg",
            "targetFrame": self.totalFrames
        }

def getTestCasesFromStatusMd():
    testCases = {}
    # Read STATUS.md and parse test cases to dictionary
    with open('STATUS.md', 'r', encoding='utf-8') as f:
        # Skip the first 4 lines to get to first test case line
        for i in range(4):
            next(f)

        for line in f:
            testCase = TestCase(line)
            testCases[testCase.name] = testCase
    return testCases

# e.g. Get "rr-ng-rta-2-24-281" from [TestDirector.cc:112] Test Case Passed: rr-ng-rta-2-24-281 [1207 / 9060]
def getTestCaseNameFromStdout(line):
    return line.split(':')[2].split(' ')[1]

# e.g. Get the "814" from [TestDirector.hh:55] Test Case Failed: rr-ng-rta-2-24-281 [814 / 9060]
def getTestCaseFrameLowerBoundFromStdout(line):
    return int(line.split(':')[2].split('[')[1].split(' ')[0])

def validateResults(testCases, resultStr):
    isSuccess = True
    for line in resultStr.split('\n'):
        # Skip any desync debugging lines
        if "Test Case" not in line:
            continue

        testCaseName = getTestCaseNameFromStdout(line)
        testCase = testCases[testCaseName]
        desyncFrame = getTestCaseFrameLowerBoundFromStdout(line)

        # If the desync frame is higher than the current frame count, we will need to return False
        if (desyncFrame - 1 != testCase.syncFrame and desyncFrame != testCase.totalFrames):
            isSuccess = False

            # Our framecount isn't up-to-date - the test case desyncs either later or earlier than captured in STATUS.md
            if (desyncFrame - 1 < testCase.syncFrame):
                print(f"{testCaseName.ljust(20)}\t{testCase.syncFrame}\t->\t{desyncFrame - 1}\t[EARLY]")
            else:
                print(f"{testCaseName.ljust(20)}\t{testCase.syncFrame}\t->\t{desyncFrame - 1}\t[LATER]")
        
    return isSuccess

def toJson(testCases):
    testCaseDict = {}
    for key, value in testCases.items():
        testCaseDict[key] = value.toJson()

    return testCaseDict

def main():
    # Generate the upper bound test case JSON
    testCases = getTestCasesFromStatusMd()

    # Write out the test cases to a JSON file
    with open(STATUS_TEST_CASE_FILENAME, 'w') as f:
        f.write(json.dumps(toJson(testCases), indent=4))

    # Generate the binary file
    generate_tests(STATUS_TEST_CASE_FILENAME)

    # Run Kinoko. subprocess.run shell behavior varies between Windows and Linux
    try:
        import msvcrt
    except ModuleNotFoundError:
        result = subprocess.run("./kinoko -s testCases.bin", cwd='out', shell=True, capture_output=True, text=True)
    else:
        result = subprocess.run([".\\kinoko", "-s", "testCases.bin"], cwd='out', shell=True, capture_output=True, text=True)
    
    
    # Check each test case is up-to-date. If not, return non-zero exit code so our build action is aware.
    if not validateResults(testCases, result.stdout):
        sys.exit(1)

if __name__ == '__main__':
    main()