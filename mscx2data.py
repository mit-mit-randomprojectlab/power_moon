#!/usr/bin/python

"""
mscx2data.py: Converts uncompressed Musescore score files with two parts into header files
containing data that can be played in the Power Moon arduino code
"""

import xml.etree.ElementTree as ET

# Paths to data
input_path = 'fossil_falls.mscx'
output_path = 'fossil_falls.h'

# create delay numbers for duration strings
count = {'32nd': 3, '16th': 6, 'eighth': 12, 'quarter': 24, 'half': 48, 'whole': 96, 'measure': 96}

# Open XML formatted mscx file for score
tree = ET.parse(input_path)
root = tree.getroot()

# get score and parts
for child in root:
	if child.tag == 'Score':
		score = child
		for child2 in score:
			if child2.tag == 'Staff':
				if child2.attrib['id'] == '1':
					part1 = child2
				elif child2.attrib['id'] == '2':
					part2 = child2

# cycle through each part and pull out data
tied = False
music_data_A = []
for measure in part1.iter('Measure'):
	for child in measure:
		if child.tag == 'Chord':
			is_triplet = (len([i for i in child.iter('Tuplet')]) == 1)
			is_tie = (len([i for i in child.iter('Tie')]) == 1)
			is_dot = (len([i for i in child.iter('dots')]) == 1)
			duration = [i for i in child.iter('durationType')][0].text
			delay = count[duration]
			if is_triplet:
				delay = int(2*delay/3)
			if is_dot:
				delay = int(1.5*delay)
			pitch = int([j for j in [i for i in child.iter('Note')][0].iter('pitch')][0].text)
			if tied:
				music_data_A[-1][1] += delay
			else:
				music_data_A.append([pitch, delay])
			if is_tie:
				tied = True
			else:
				tied = False
		elif child.tag == 'Rest':
			is_triplet = (len([i for i in child.iter('Tuplet')]) == 1)
			duration = [i for i in child.iter('durationType')][0].text
			delay = count[duration]
			if is_triplet:
				delay = int(2*delay/3)
			music_data_A.append([0, delay])

tied = False
music_data_B = []
for measure in part2.iter('Measure'):
	for child in measure:
		if child.tag == 'Chord':
			is_triplet = (len([i for i in child.iter('Tuplet')]) == 1)
			is_tie = (len([i for i in child.iter('Tie')]) == 1)
			is_dot = (len([i for i in child.iter('dots')]) == 1)
			duration = [i for i in child.iter('durationType')][0].text
			delay = count[duration]
			if is_triplet:
				delay = int(2*delay/3)
			if is_dot:
				delay = int(1.5*delay)
			pitch = int([j for j in [i for i in child.iter('Note')][0].iter('pitch')][0].text)
			if tied:
				music_data_B[-1][1] += delay
			else:
				music_data_B.append([pitch, delay])
			if is_tie:
				tied = True
			else:
				tied = False
		elif child.tag == 'Rest':
			is_triplet = (len([i for i in child.iter('Tuplet')]) == 1)
			duration = [i for i in child.iter('durationType')][0].text
			delay = count[duration]
			if is_triplet:
				delay = int(2*delay/3)
			music_data_B.append([0, delay])

# Save out data as header file
f = open(output_path, "w");
f.write('// Song data\n')
f.write('\n')

f.write('int musicA_count = %d;\n'%(len(music_data_A)))
f.write('int musicB_count = %d;\n'%(len(music_data_B)))
f.write('\n')

f.write('uint8_t musicA_pitch[] = {')
c = 0
for d in music_data_A:
	f.write('%d,'%(d[0]))
	c += 1
	if c > 30:
		c = 0
		f.write('\n')
f.write('};\n')
f.write('\n')
c = 0
f.write('uint8_t musicA_duration[] = {')
for d in music_data_A:
	f.write('%d,'%(d[1]))
	c += 1
	if c > 30:
		c = 0
		f.write('\n')
f.write('};\n')
f.write('\n')
c = 0
f.write('uint8_t musicB_pitch[] = {')
for d in music_data_B:
	f.write('%d,'%(d[0]))
	c += 1
	if c > 30:
		c = 0
		f.write('\n')
f.write('};\n')
f.write('\n')
c = 0
f.write('uint8_t musicB_duration[] = {')
for d in music_data_B:
	f.write('%d,'%(d[1]))
	c += 1
	if c > 30:
		c = 0
		f.write('\n')
f.write('};\n')

f.close()



