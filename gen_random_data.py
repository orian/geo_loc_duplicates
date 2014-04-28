#!/usr/bin/python

import argparse
import copy
import random

def GeneratePoints(x_dim, y_dim, num_points):
	results = []
	for i in xrange(num_points):
		results.append((random.randrange(0, x_dim), random.randrange(0, y_dim)))
	return results

# the GCD(len(HOTEL_EXTRA_WORD_LIST), len(HOTEL_WORD_LIST)) == 0
HOTEL_EXTRA_WORD_LIST = 'inn,carriage,launge,plaza,blue,red,green'
HOTEL_WORD_LIST = 'sheraton,hilton,mariott,intercontinental,grand,rex,orhid,radisson,valamar,ibis,menteleone'

def NameGen(base,extra,num):
	i = 0
	e_idx = 0
	b_idx = 0
	while i != num:
		yield '{base} {extra}'.format(base=base[b_idx],extra=extra[e_idx])
		i = i+1
		e_idx = e_idx + 1
		b_idx = b_idx + 1
		if e_idx == len(extra):
			e_idx = 0
		if b_idx == len(base):
			b_idx = 0

def MakeNames(geo_loc_list):
	extra = HOTEL_EXTRA_WORD_LIST.split(',')
	base = HOTEL_WORD_LIST.split(',')
	results = []
	for (x,y),name in zip(geo_loc_list, NameGen(base,extra,len(geo_loc_list))):
		results.append({'x':x, 'y':y, 'name':name})
	return results

def FuzzAndDuplicate(geo_list, fuzz_pbb=0.5):
	results = [copy.deepcopy(g) for g in geo_list]
	h_id = 1
	e_id = h_id + len(results)
	for g in results:
		g['id'] = h_id
		g['unique_id'] = h_id
		h_id += 1

	for g in results:
		if random.random() < fuzz_pbb:
			gc = copy.deepcopy(g)
			gc['id'] = h_id
			results.append(gc)
			h_id += 1
	# fuzzing
        # note: because of it, the point and it's origin may be in distance 2*a
        a = 0.1
	for r in results:
		#r['x'] = random.gauss(r['x'], 0.1)
		#r['y'] = random.gauss(r['y'], 0.1)
		r['x'] = r['x'] + random.uniform(-a, a)
		r['y'] = r['y'] + random.uniform(-a, a)
	return results

def SaveToFile(filename, geo_list):
	with open(filename, 'w') as f:
		for o in geo_list:
			f.write('{id},{unique_id},{x},{y},{name}\n'.format(**o))

if __name__ == '__main__':
	parser = argparse.ArgumentParser(description='Generate random points close to a grid lines of a given size.')
	parser.add_argument('x_dim', type=int, help='grid horizontal size')
	parser.add_argument('y_dim', type=int, help='grid vertical size')
	parser.add_argument('num_points', type=int, help='number of points')
	parser.add_argument('fuzz_pbb', type=float, help='ppb of item duplication')
        parser.add_argument('filename', type=str, help='output data filename')
	args = parser.parse_args()
	l = GeneratePoints(args.x_dim, args.y_dim, args.num_points)
	print 'Generated {size} number of points'.format(size=len(l))
	l = MakeNames(l)
	print 'Gave names for {size} geo locations.'.format(size=len(l))
	l = FuzzAndDuplicate(l, args.fuzz_pbb)
	print 'Duplicated and fuzzied {size}.'.format(size=len(l))
	SaveToFile(args.filename, l)
