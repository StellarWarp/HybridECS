import json
import os
import subprocess
import matplotlib.pyplot as plt
import re


# 运行Google Benchmark并捕获输出
def run_benchmark(executable, options):
    command = f"{executable} {options}"
    process = subprocess.run(command, shell=True, text=True)
    if process.returncode == 0:
        print("Subprocess executed successfully")
    else:
        raise Exception(f"Subprocess failed with exit code {process.returncode}")





# 主函数
def main():
    print(os.getcwd())
    benchmark_file = 'results.json'
    executable = os.getcwd() + r"\build\windows\x64\release\Delegate.exe"
    options = f"--benchmark_repetitions=5 --benchmark_format=json --benchmark_out={benchmark_file}"

    #check exist for executable
    if not os.path.exists(executable):
        print(f"Executable {executable} not found")
        exit(1)

    try:
        run_benchmark(executable, options)
    except Exception as e:
        print(e)
        exit(1)


    with open(benchmark_file, 'r') as file:
        data = json.load(file)

        res = {}
        for benchmark in data['benchmarks']:
            name = benchmark['name'].split('/')[0]
            if benchmark['run_type'] == 'iteration':
                if name not in res:
                    res.setdefault(name, 
                                     {'cpu_time': [],
                                    'iterations': [],
                                    'mean': 0,
                                    'median': 0,
                                    'stddev': 0,
                                    'cv': 0
                                       })

                res[name]['cpu_time'].append(benchmark['cpu_time'])
                res[name]['iterations'].append(benchmark['iterations'])
            elif benchmark['run_type'] == 'aggregate':
                match benchmark['aggregate_name']:
                    case 'mean':
                        res[name]['mean'] = benchmark['cpu_time']
                    case 'median':
                        res[name]['median'] = benchmark['cpu_time']
                    case 'stddev':
                        res[name]['stddev'] = benchmark['cpu_time']
                    case 'cv':
                        res[name]['cv'] = benchmark['cpu_time']
            
        names = list(res.keys())
        means = [res[name]['mean'] for name in names]
        stddevs = [res[name]['stddev'] for name in names]

        plt.bar (names, means, yerr=stddevs, color='blue', alpha=0.7)
        plt.xlabel('Benchmark Name')
        plt.ylabel('CPU Time (with error bars)')
        plt.title('Benchmark Performance')
        plt.xticks(rotation=80, ha='right')
        plt.tight_layout()
        plt.show()
        

if __name__ == '__main__':
    main()
