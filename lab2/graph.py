import pandas as pd
import matplotlib.pyplot as plt


df = pd.read_csv('data.txt')
t1 = df.iloc[0]['ms']
df['E'] = t1 / (df['P'] * df['ms'])
df['S'] = t1 / df['ms']
print(df)

'''
plt.plot(df['P'], df['ms'], marker='o')
plt.title('Mjerenje trajanja računanja poteza')
plt.xlabel("Broj procesora")
plt.ylabel("Trajanje [ms]")
plt.show()
'''

'''
plt.plot(df['P'], df['S'], marker='^', label='izmjereno')
plt.plot(df['P'], df['P'], marker='s', label='idealno')
plt.title('Ubrzanje')
plt.xlabel("Broj procesora")
plt.ylabel("Ubrzanje")
plt.legend(loc='best')
plt.show()
'''

'''
plt.plot(df['P'], df['E'], marker='o')
plt.title('Učinkovitost')
plt.xlabel("Broj procesora")
plt.ylabel("Učinkovitost")
plt.show()
'''

