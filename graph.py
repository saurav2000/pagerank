import matplotlib.pyplot as plt
import numpy

names = ['20k', '30k', '40k', '50k', '60k', '70k', '80k', '90k', '100k']
time_cpp = [1.81, 2.93, 3.17, 4.49, 5.08, 6.97, 8.55, 9.12, 10.87]
time_mpi1 = [1.01, 1.56, 1.63, 2.28, 2.46, 3.37, 3.96, 4.19, 5.21]
time_mpi2 = [0.8, 1.13, 1.27, 1.67, 1.87, 2.53, 2.94, 3.1, 3.77]
time_mpi4 = [0.77, 1.10, 1.2, 1.58, 1.75, 2.24, 2.63, 2.8, 3.38]

time_mpi_base1 = [0.95, 1.22, 1.27, 1.63, 1.81, 2.38, 2.74, 2.87, 3.49]
time_mpi_base2 = [0.83, 1.05, 1.11, 1.37, 1.47, 1.88, 2.17, 2.30, 2.71]
time_mpi_base4 = [0.77, 0.96, 1.01, 1.23, 1.33, 1.76, 1.88, 1.95, 2.27]

x = numpy.arange(9)
fig = plt.figure()
ax = fig.add_subplot(111)
width = 0.3
rects1 = ax.bar(x, time_mpi_base1, width, color='r')
rects2 = ax.bar(x + width, time_mpi_base2, width, color='g')
rects3 = ax.bar(x + 2*width, time_mpi_base4, width, color='b')
ax.legend((rects1[0], rects2[0], rects3[0]), ('1 core', '2 cores', '4 cores'))
ax.set_xticks(x + width)
ax.set_xticklabels(names)
# ax.bar(x, time_cpp)
# ax.plot(x, time_cpp)
plt.show()
