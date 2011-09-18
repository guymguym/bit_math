#ifndef BIT_MATH__H__
#define BIT_MATH__H__

#include <stdint.h>
#include <assert.h>
#include <cstdlib>
#include <cstring>

#include <deque>

#include <iostream>
#include <iomanip>
#include <ios>

#define DO1(X) do { X; } while(0)
#define DO2(X) DO1(X); DO1(X)
#define DO4(X) DO2(X); DO2(X)
#define DO8(X) DO4(X); DO4(X)
#define DO(__N__, X) \
	do { \
		int __n__ = __N__; \
		while (__n__ >= 8)  { DO8(X); __n__ -= 8; } \
		while (__n__ >= 4)  { DO4(X); __n__ -= 4; } \
		while (__n__ >= 2)  { DO2(X); __n__ -= 2; } \
		while (__n__ >= 1)  { DO1(X); __n__ -= 1; } \
	} while(0)


namespace bit_math {


/*
 * Simple mechanism for COW (copy-on-write) for shared data
 */
template <class T>
class SharedData 
{
private:
	int _shares;
public:
	T data;
	SharedData() 
		: _shares(0) {}
	SharedData(SharedData& other) 
		: _shares(0)
		, data(other.data) 
		{ other.remove_share(); }
	bool is_shared() { return _shares > 0; }
	void add_share() { ++_shares; }
	void remove_share() { --_shares; }
};


/*
 * Wrapper class to represent a binary 'bit' and its native operations
 */
class Bit
{
private:
	unsigned char _val : 1;
public:
	Bit() : _val(0) {}
	Bit(bool val) : _val(val ? 1 : 0) {}
	Bit(int val)  : _val(val ? 1 : 0) {}
	Bit(const Bit& bit) : _val(bit._val) {}
	operator bool() const { return _val ? true : false; }
	operator int() const { return _val ? 1 : 0; }
	const Bit&  operator=(const Bit&  bit) { _val = bit._val;    return bit; }
	const bool& operator=(const bool& val) { _val = val ? 1 : 0; return val; }
	const int&  operator=(const int&  val) { _val = val ? 1 : 0; return val; }
	bool operator==(const Bit& bit) const { return _val == bit._val; }
	bool operator!=(const Bit& bit) const { return _val != bit._val; }
	bool operator<(const Bit& bit) const { return _val < bit._val; }
	bool operator<=(const Bit& bit) const { return _val <= bit._val; }
	bool operator>(const Bit& bit) const { return _val > bit._val; }
	bool operator>=(const Bit& bit) const { return _val >= bit._val; }
	Bit operator!() const { return _val ? Bit(0) : Bit(1); }
	int to_sign() const { return _val ? -1 : 1; }
};


/*
 * Int represents an arbitrarily large integral value (signed)
 * The template of WORD must be unsigned type but can be on any integral size (8bit - 64bit)
 */
template <class WORD = unsigned char>
class Int
{
private:
	typedef WORD Word;
	typedef uint32_t Index;
	enum { 
		BITS_IN_BYTE = 8u,
		WORD_SIZE = sizeof(Word), 
		WORD_BITS = BITS_IN_BYTE * WORD_SIZE
	};
	enum {
		WORD_ONE = Word(1u),
		WORD_MASK = Word(-1u)
	};
	// using deque instead of vector to avoid large memory allocations
	typedef std::deque<Word> Vec;
	typedef SharedData<Vec> SharedVec;
	SharedVec* _data;
	Bit _sign;

private:
	void prepare_for_write(bool cow) {
		if (!_data) {
			_data = new SharedVec;
		} else if (_data->is_shared()) {
			if (cow) {
				SharedVec* new_data = new SharedVec(*_data);
				_data = new_data;
			} else {
				_data = new SharedVec;
			}
		}
	}
	
	Vec& wvec(bool cow = true) { 
		prepare_for_write(cow); 
		return _data->data; 
	}
	const Vec& vec() const { 
		static const Vec V;
		return _data ? _data->data : V;
	}
	Index nwords() const { return vec().size(); }

	const Word& get_word(Index word) const { 
		static const Word Z = 0;
		return word < nwords() ? vec()[word] : Z;
	}

	void set_word(Index word, Word val) { 
		if (val != 0) {
			if (word >= nwords()) 
				wvec().resize(word+1, 0);
			wvec()[word] = val;
			return;
		}
		// val == 0, no need to resize up for it
		if (word >= nwords()) 
			return;
		wvec()[word] = 0; 
		// try truncating down the vec by coalescing zeros as much as possible
		if (word + 1 == nwords()) {
			while (word > 0 && vec()[word-1] == 0) --word;
			wvec().resize(word);
			if (word == 0) _sign = 0; // clear sign for zero as well to avoid effects laterz
		}
	}

	void push_front_word(Word val) {
		wvec().push_front(val); // efficient
	}

	void pop_front_word() {
		wvec().pop_front(); // efficient
	}

	void clear() { 
		wvec(false).clear(); // don't copy-on-write for clear
		_sign = 0; 
	}
	
	template <class T> 
	static Word cut_word(T& val) {
		Word w = val & WORD_MASK;
		val >>= WORD_BITS;
		return w;
	}

	template <class T> 
	void init(T val) {
		_sign = (val < 0);
		int i = 0;
		DO((sizeof(val)+WORD_SIZE-1)/WORD_SIZE, set_word(i++,cut_word(val)));
	}

	void init(const Int& num) {
		_sign = num._sign;
		_data = const_cast<SharedVec*>(num._data);
		if (_data) _data->add_share();
	}

	void release() {
		if (!_data) return;
		if (_data->is_shared()) 
			_data->remove_share(); 
		else 
			delete _data;
		_data = 0;
	}

public:
	// constructors
	Int() : _data(0) {}
	Int(uint32_t val) { init(val); }
	Int(uint64_t val) { init(val); }
	Int(int32_t  val) { init(val); }
	Int(int64_t  val) { init(val); }
	Int(const Int& num) { init(num); } // copy-ctor
	~Int() { release(); }

	const Int& operator=(const Int& num) { 
		if (this != &num) { 
			release(); 
			init(num); 
		} 
		return num; 
	}

	bool is_zero() const	{ return vec().empty(); }
	Bit get_sign() const	{ return _sign; }
	void set_sign(Bit sign) { _sign = sign; }
	void toggle_sign()		{ _sign = !_sign; }

	Bit get_bit(Index bit) const {
		const Index word = bit / WORD_BITS;
		const Word pos = WORD_ONE << (bit % WORD_BITS);
		const Word& w = get_word(word);
		return w & pos;
	}
	void set_bit(Index bit, Bit val) {
		const Index word = bit / WORD_BITS;
		const Word pos = WORD_ONE << (bit % WORD_BITS);
		const Word& w = get_word(word);
		set_word(word, val ? (w|pos) : (w&~pos));
	}

	void lshift(Index n) { lshift(n,0); }
	void lshift(Index n, Bit insert) {
		while (n >= WORD_BITS) {
			push_front_word(insert ? WORD_MASK : 0);
			n-= WORD_BITS;
		}
		lshift_word(n, WORD_MASK);
	}
	void lshift_word(Index n, Word val) {
		assert(n <= WORD_BITS);
		if (n <= 0) 
			return;
		if (n >= WORD_BITS) {
			push_front_word(val);
			return;
		}
		const Index nrev = WORD_BITS - n;
		Word keep = val & (WORD_MASK >> nrev);
		const Index len = nwords();
		for (Index i = 0; i < len || keep != 0; ++i) {
			const Word& x = get_word(i);
			Word keep_next = x >> nrev;
			set_word(i, (x << n) | keep);
			keep = keep_next;
		}
	}
	
	void rshift(Index n) {
		while (n >= WORD_BITS) {
			pop_front_word();
			n-= WORD_BITS;
		}
		if (n <= 0) return;
		const Index nrev = WORD_BITS - n;
		const Word nmask = WORD_MASK >> nrev;
		Word keep = 0;
		const Index len = nwords();
		for (Index i = 0; i < len; ++i) {
			const Word& x = get_word(len-1-i);
			Word keep_next = x & nmask;
			set_word(i, (x >> n) | (keep << nrev));
			keep = keep_next;
		}
	}
	
	void plus(const Int& num) {
		Word carry = 0;
		const Index mylen = nwords();
		const Index len = num.nwords();
		if (len == 0) return;
		int cs = (_sign==num._sign) ? 0 : 1;
		if (cs && (mylen < len || ( mylen==len && get_word(len-1) < num.get_word(len-1) ))) {
			cs = 2;
		}
		cout << endl << "len=" << len << " cs=" << cs << " this=" << *this << " num=" << num;
		for (Index i = 0; i < len || carry != 0; ++i) {
			assert(i <= len);
			Word x = get_word(i);
			Word y = num.get_word(i);
			cout << endl << std::hex << "x=" << int(x) << " y=" << int(y) << " carry=" << int(carry);
			Word z;
			switch (cs) {
				case 0: // x + y
					x += carry;
					z = x + y;
					// carry on unsigned overflow
					carry = (z < x) ? 1 : 0;
					break;
				case 1: // x - y and this>=num
					x -= carry;
					z = x - y;
					carry = (z > x) ? 1 : 0;
					break;
				case 2: // x - y and this<num
					y -= carry;
					z = y - x;
					carry = (z > y) ? 1 : 0;
					break;
			}
			cout << " z=" << int(z);
			set_word(i, z);
		}
		if (cs == 2) toggle_sign();
		cout << endl << std::dec;
	}

	void mult(const Int& num) {}
	void div(const Int& num) {}
	void mod(const Int& num) {}

	Int operator-() const { Int i(*this); i.toggle_sign(); return i; }
	Int operator+(const Int& num) const { Int i(*this); i.plus(num); return i; }
	Int operator-(const Int& num) const { Int i(*this); i.plus(-num); return i; }
	Int operator*(const Int& num) const { Int i(*this); i.mult(num); return i; }
	Int operator/(const Int& num) const { Int i(*this); i.div(num); return i; }
	Int operator%(const Int& num) const { Int i(*this); i.mod(num); return i; }
	Int operator>>(int n) const { Int i(*this); i.rshift(n); return i; }
	Int operator<<(int n) const { Int i(*this); i.lshift(n); return i; }
	
	
	bool parse(const std::string& str) {
		clear();
		int base = 10;
		Bit sign = 0;
		size_t i = 0;
		// check for minus sign
		if (i < str.size() && str[i] == '-') {
			sign = 1; // keep sign for later to avoid set_word from zeroing it
			++i;
		}
		// check the base
		if (i+1 < str.size() && str[i] == '0' && str[i+1] == 'b') {
			base = 2;
			i += 2;
		} else if (i+1 < str.size() && str[i] == '0' && str[i+1] == 'x') {
			base = 16;
			i += 2;
		} else if (i < str.size() && str[i] == '0') {
			base = 8;
			i += 1;
		}

		switch (base) {

			case 2: {
				int bits = 0;
				Word word = 0;
				for (; i < str.size(); ++i) {
					if (str[i] < '0' || str[i] > '1')
						return false;
					int bin_val = str[i] - '0';
					if (bits + 1 > WORD_BITS) {
						lshift_word(bits, word);
						bits = 0;
						word = 0;
					}
					bits += 1;
					word = (word << 1) | bin_val;
				}
				if (bits) {
					lshift_word(bits, word);
				}
				break;
			}
		
			case 8: {
				int bits = 0;
				Word word = 0;
				for (; i < str.size(); ++i) {
					if (str[i] < '0' || str[i] > '7')
						return false;
					int oct_val = str[i] - '0';
					if (bits + 3 > WORD_BITS) {
						lshift_word(bits, word);
						bits = 0;
						word = 0;
					}
					bits += 3;
					word = (word << 3) | oct_val;
				}
				if (bits) {
					lshift_word(bits, word);
				}
				break; 
			}
		
			case 16: {
				int bits = 0;
				Word word = 0;
				for (; i < str.size(); ++i) {
					int hex_val;
					if (str[i] >= '0' && str[i] <= '9')
						hex_val = str[i] - '0';
					else if (str[i] >= 'a' && str[i] <= 'f')
						hex_val = 10 + str[i] - 'a';
					else if (str[i] >= 'A' && str[i] <= 'F')
						hex_val = 10 + str[i] - 'A';
					else
						return false;
					if (bits + 4 > WORD_BITS) {
						lshift_word(bits, word);
						bits = 0;
						word = 0;
					}
					bits += 4;
					word = (word << 4) | hex_val;
				}
				if (bits) {
					lshift_word(bits, word);
				}
				break;
			}

			case 10: // TODO parse decimal
			default:
				return false;
		}

		set_sign(sign);
		return true;
	}

	friend std::ostream& operator<<(std::ostream& os, const Int& num) {
		// speedy handling for zero - this is why we'll never get base specifiers for zero.
		if (num.is_zero()) 
			return os << '0';
		if (num.get_sign())
			os << '-';

		std::ios_base::fmtflags base = os.flags() & std::ios_base::basefield;
		const Index len = num.nwords();

		switch (base) {

			case std::ios_base::dec: // TODO print decimal
			case std::ios_base::hex: {
				os << "0x";
				for (Index i = 0; i<len; ++i) {
					const Word& x = num.get_word(len-1-i);
					for (int k=WORD_BITS-BITS_IN_BYTE; k>=0; k-=BITS_IN_BYTE) {
						Word vh = (x >> (k+4)) & 0xF;
						Word vl = (x >> k) & 0xF;
						os << char(vh<10 ? '0'+vh : 'a'+vh-10);
						os << char(vl<10 ? '0'+vl : 'a'+vl-10);
					}
				}
				break;
			}

			case std::ios_base::oct: {
				os << '0';
				// TODO print octal not working right
				//int bits = 3 - (len*WORD_BITS) % 3;
				//int oct = 0;
				for (Index i = 0; i<len; ++i) {
					const Word& x = num.get_word(len-1-i);
					for (int k=WORD_BITS-BITS_IN_BYTE; k>=0; k-=BITS_IN_BYTE) {
						Word vh = (x >> (k+6)) & 0x3;
						Word vm = (x >> (k+3)) & 0x7;
						Word vl = (x >> k) & 0x7;
						os << char('0'+vh) << char('0'+vm) << char('0'+vl);
					}
				}
				break;
			}

			default:
				break;
		}
		return os;
	}

};


} // namespace bit_math

#endif // BIT_MATH__H__

