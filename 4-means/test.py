import random
import subprocess

def test(iter_cnt=10):
    for i in range(iter_cnt):
        nums_cnt = random.randint(4,12)
        print(f'____TEST {i}____')
        print('Numbers count:', nums_cnt)
        process = subprocess.run(['bash', 'test.sh', str(nums_cnt)], capture_output=True, text=True)
        output_lines = process.stdout.strip().split('\n')
        results = [(float(line[1:line.find("]")]), 
                    line.find("]") - (line.find(".") + 1 if line.find(".") != -1 else line.find("]")), 
                    [int(num) for num in line[line.find(" "):].replace(' ', '').split(',')]) 
                    for line in output_lines]
        in_file = open('numbers', 'rb')
        nums = []
        for i in range(nums_cnt):
            nums.append(int.from_bytes(in_file.read(1), "big"))
        clusters = [0 for i in range(len(nums))]
        ms = nums[:4]

        print('Numbers:', nums)

        while(True):
            for i, num in enumerate(nums):
                distance = 256
                cluster = 0
                for j, m in enumerate(ms):
                    dist = abs(num - m)
                    if dist < distance:
                        distance = dist
                        cluster = j
                clusters[i] = cluster

            nclusters = [[num for i, num in enumerate(nums) if clusters[i] == j] for j in range(4)]
            new_ms = [sum(cluster) / len(cluster) for cluster in nclusters]

            if new_ms == ms:
                fail = False
                for i, m in enumerate(new_ms):
                    if (nclusters[i] != results[i][2] or round(m,results[i][1]) != results[i][0]):
                        print('FAIL')
                        fail = True
                
                if fail:
                    print('Expected:')
                    for i, m in enumerate(new_ms):
                        print(f'[{round(m,results[i][1])}] ', end='')
                        for j, num in enumerate(nclusters[i]):
                            if (j < len(nclusters[i]) - 1):
                                print(f'{num},', end=' ')
                            else:
                                print(f'{num}')
                    print('Got:')
                    for i in range(len(output_lines)):
                        print(output_lines[i])
                else:
                    print('PASS')
                    
                break
            ms = new_ms


if __name__ == '__main__':
    test(iter_cnt=10)