import random
import subprocess as sp

def xor_plus(x,y):
	x1 = x^y
	y1 = (x&y) << 1
	while x1&y1:
		t = x1
		x1 = x1^y1
		y1 = (t&y1) << 1
	z = (x1^y1)
	if x+y != z:
		print hex(x), hex(y), hex(x+y), hex(z)
		return False
	return True

# spawn the msh process
msh = sp.Popen(["./msh", "--pipe"], stdin=sp.PIPE, stdout=sp.PIPE, bufsize=4096)
ipipe, opipe = msh.stdin, msh.stdout
rd = opipe.readline().strip()
if rd != "<msh>":
	print "TEST FAILED: Expected <msh> prompt, got", rd
	exit(1)

for i in xrange(100000):
	
	x = random.randint(0,0xffffffffffff)
	y = random.randint(0,0xffffffffffff)
	signs = random.randint(0,3)
	if signs == 1:   x = -x
	elif signs == 2: y = -y
	elif signs == 3: x,y = -x, -y
	
	if i % 100 == 0: print i, signs

	xs, ys = str(hex(x)), str(hex(y))
	if type(x) is long: xs = xs[0:-1]
	if type(y) is long: ys = ys[0:-1]
	
	print >>ipipe, xs, "+", ys
	ipipe.flush()
	rd = ""
	while rd != "<msh>":
		out = rd
		rd = opipe.readline().strip()
	z = long(out.strip(),16)
	
	if z != x+y:
		print "TEST FAILED: x", hex(x), "y", hex(y), "x+y", hex(x+y), "z", hex(z)
		print out
		break

