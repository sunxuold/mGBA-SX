



1024 1024   733  1024
1024  768   866  15xx
1024  624   1013  1248
1024  512   4xx   1024
1024  640   888   1280

	m->offset     = m->factor / 2;
	m->avail      = 0;
	m->integrator = 0;

blip_set_rates(audio->psg.left, GBA_ARM7TDMI_FREQUENCY, 96000);
              

                    16777216                    44100                  1        
blip_set_rates(left, clockRate, audioContext->obtainedSpec.freq * fauxClock);

void blip_set_rates( blip_t* m, double clock_rate, double sample_rate )
	double factor = time_unit * 44100 / 16777216;
		m->factor = (fixed_t) factor;
	
if ( m->factor < factor )
		m->factor++;


	fixed_t off = t * m->factor + m->offset;
	m->avail += off >> time_bits;
	m->offset = off & (time_unit - 1);


return m->avail;



blip_t* left = NULL;

left = audioContext->core->getAudioChannel(audioContext->core, 0);



_defaultFPSTarget = 60.f;
fauxClock = GBAAudioCalculateRatio(1, audioContext->sync->fpsTarget, 1);   ~0.995

#define GBA_ARM7TDMI_FREQUENCY 0x1000000U   16M~xxxx                                             
VIDEO_TOTAL_LENGTH = 280896,				274K 	 							
float GBAAudioCalculateRatio(float inputSampleRate, float desiredFPS, float desiredSampleRate) {
	return desiredSampleRate * GBA_ARM7TDMI_FREQUENCY / (VIDEO_TOTAL_LENGTH * desiredFPS * inputSampleRate);
	

	
					 16777216		  	        44100                1
blip_set_rates(left, clockRate, audioContext->obtainedSpec.freq * fauxClock);

pre_shift = 0/32 ?
time_bits = pre_shift + 19

time_unit = (fixed_t) 1 << time_bits;

blip_max_ratio = 0x100000 
             1(19)(20)     1(20)
m->factor = time_unit / blip_max_ratio;
     
void blip_set_rates( blip_t* m, double clock_rate, double sample_rate )
{
                     1(20)          44100        16M 16 *1(20) 
	double factor = time_unit * sample_rate / clock_rate;  =44100/16
	m->factor = (fixed_t) factor;
	
	/* Fails if clock_rate exceeds maximum, relative to sample_rate */
	assert( 0 <= factor - m->factor && factor - m->factor < 1 );
	
	/* Avoid requiring math.h. Equivalent to
	m->factor = (int) ceil( factor ) */
	if ( m->factor < factor )
		m->factor++;      44100/16
	
	/* At this point, factor is most likely rounded up, but could still
	have been rounded down in the floating-point calculation. */
}




                      2
len /= 2 * audioContext->obtainedSpec.channels;


int available = blip_samples_avail(left);     !!!!!!!!!!!!!!!

int blip_samples_avail( const blip_t* m )
{
	return m->avail;
}

audio->left = blip_new(BLIP_BUFFER_SIZE);



struct blip_t
{
	fixed_t factor;
	fixed_t offset;
	int avail;
	int size;
	int integrator;
};

typedef int buf_t
{ end_frame_extra = 2 };
{ half_width  = 8 }
buf_extra   = half_width*2 + end_frame_extra 

blip_max_ratio = 0x100000

                     0x4000;
blip_t* blip_new( int size )
{
	blip_t* m;
	assert( size >= 0 );
	
	m = (blip_t*) malloc( sizeof *m + (size + buf_extra) * sizeof (buf_t) );
	if ( m )
	{                1(20)     1(20) 0x100000 
		m->factor = time_unit / blip_max_ratio;
		m->size   = size;
		blip_clear( m );
		check_assumptions();
	}
	return m;
}

                                  0x1000
void blip_end_frame( blip_t* m, unsigned t )
{                1(12)   44100/16~1()                0               
	fixed_t off = 0x1000 * 44100/16 * m->factor + m->offset;
	                    (20)   
	m->avail += off >> 20;   = 10.7666
	m->offset = off & (time_unit - 1);
	
	/* Fails if buffer size was exceeded */
	assert( m->avail <= m->size );
}


void blip_clear( blip_t* m )
{
	/* We could set offset to 0, factor/2, or factor-1. 0 is suitable if
	factor is rounded up. factor-1 is suitable if factor is rounded down.
	Since we don't know rounding direction, factor/2 accommodates either,
	with the slight loss of showing an error in half the time. Since for
	a 64-bit factor this is years, the halving isn't a problem. */
	
	m->offset     = m->factor / 2;  
	m->avail      = 0;
	m->integrator = 0;
	memset( SAMPLES( m ), 0, (m->size + buf_extra) * sizeof (buf_t) );
}




SAMPLES( buf ) ((buf_t*) ((buf) + 1))

blip_read_samples(left, (short*) data, available, audioContext->obtainedSpec.channels == 2);

int blip_read_samples( blip_t* m, short out [], int count, int stereo )
{
	assert( count >= 0 );
	
	if ( count > m->avail )
		count = m->avail;
	
	if ( count )
	{
		int const step = stereo ? 2 : 1;
		buf_t const* in  = SAMPLES( m );
		buf_t const* end = in + count;
		int sum = m->integrator;
		do
		{
			/* Eliminate fraction */ // delta_bits  = 15 
			#define ARITH_SHIFT( n, shift ) \
	((n) >> (shift))
	
	
			int s = ARITH_SHIFT( sum, delta_bits );
			
			sum += *in++;
			
			CLAMP( s );
			
			*out = s;
			out += step;
			
			/* High-pass filter */  // bass_shift  = 9 
			sum -= s << (delta_bits - bass_shift);
		}
		while ( in != end );
		m->integrator = sum;    //m->integrator = 0;
		
		remove_samples( m, count );
	}
	
	return count;
}










X86
desiredSpec info: freq 44100  format 32784 channels2  silence 0 samples 1024  padding 0 size 0
obtainedSpec info: freq 44100  format 32784 channels2  silence 0 samples 512  padding 0 size 2048
desiredSpec info: freq 44100  format 32784 channels2  silence 0 samples 512  padding 0 size 2048

init clockRate 16777216
clockRate 16777216  fauxClock 1.000000
available 1024  len 512
available 512  len 512
init clockRate 16777216
clockRate 16777216  fauxClock 1.000000
available 1029  len 512
available 512  len 512
init clockRate 16777216
clockRate 16777216  fauxClock 1.000000
available 1028  len 512
available 512  len 512
init clockRate 16777216
clockRate 16777216  fauxClock 1.000000
available 1027  len 512
available 512  len 512




GKD350  (19 /20/18/19)
desiredSpec info: freq 44100  format 32784 channels2  silence 0 samples 1024  padding 0 size 0 
obtainedSpec info: freq 44100  format 32784 channels2  silence 0 samples 1024  padding 0 size 4096 
init clockRate 16777216 
clockRate 16777216  fauxClock 1.000000 
available 571  len 1024 
available 340  len 1024
init clockRate 16777216 
clockRate 16777216  fauxClock 1.000000 
available 775  len 1024
available 781  len 1024
init clockRate 16777216 
clockRate 16777216  fauxClock 1.000000 
available 673  len 1024
available 613  len 1024
init clockRate 16777216 
clockRate 16777216  fauxClock 1.000000 
available 586  len 1024
available 592  len 1024

 (20)
desiredSpec info: freq 44100  format 32784 channels2  silence 0 samples 1024  padding 0 size 0 
obtainedSpec info: freq 44100  format 32784 channels2  silence 0 samples 1024  padding 0 size 4096 
init clockRate 16777216 
clockRate 16777216  fauxClock 1.000000 
available 340  len 1024
available 340  len 1024
init clockRate 16777216 
clockRate 16777216  fauxClock 1.000000 
available 781  len 1024
available 781  len 1024
init clockRate 16777216 
clockRate 16777216  fauxClock 1.000000 
available 613  len 1024
available 613  len 1024
init clockRate 16777216 
clockRate 16777216  fauxClock 1.000000 
available 592  len 1024
available 592  len 1024

00
01
21
22
23
24
25
26
desiredSpec info: freq 44100  format 32784 channels2  silence 0 samples 1024  padding 0 size 0 
obtainedSpec info: freq 44100  format 32784 channels2  silence 0 samples 1024  padding 0 size 4096 
init clockRate 16777216 
clockRate 16777216  fauxClock 1.000000 
available 340  len 1024
available 340  len 1024
init clockRate 16777216 
clockRate 16777216  fauxClock 1.000000 
available 781  len 1024
available 781  len 1024
init clockRate 16777216 
clockRate 16777216  fauxClock 1.000000 
available 613  len 1024
available 613  len 1024
init clockRate 16777216 
clockRate 16777216  fauxClock 1.000000 
available 592  len 1024
available 592  len 1024
00
01
21
22
23
24
25
26
desiredSpec info: freq 44100  format 32784 channels2  silence 0 samples 1024  padding 0 size 0 
obtainedSpec info: freq 44100  format 32784 channels2  silence 0 samples 1024  padding 0 size 4096 
init clockRate 16777216 
clockRate 16777216  fauxClock 1.000000 
available 349  len 1024
available 349  len 1024
init clockRate 16777216 
clockRate 16777216  fauxClock 1.000000 
available 764  len 1024
available 764  len 1024
init clockRate 16777216 
clockRate 16777216  fauxClock 1.000000 
available 786  len 1024
available 786  len 1024
init clockRate 16777216 
clockRate 16777216  fauxClock 1.000000 
available 624  len 1024
available 624  len 1024
00
01
21
22
23
24
25
26
desiredSpec info: freq 44100  format 32784 channels2  silence 0 samples 1024  padding 0 size 0 
obtainedSpec info: freq 44100  format 32784 channels2  silence 0 samples 1024  padding 0 size 4096 
init clockRate 16777216 
clockRate 16777216  fauxClock 1.000000 
available 463  len 1024
available 463  len 1024
init clockRate 16777216 
clockRate 16777216  fauxClock 1.000000 
available 710  len 1024
available 710  len 1024
init clockRate 16777216 
clockRate 16777216  fauxClock 1.000000 
available 737  len 1024
available 737  len 1024
init clockRate 16777216 
clockRate 16777216  fauxClock 1.000000 
available 376  len 1024
available 376  len 1024
00
01
21
22
23
24
25
26
desiredSpec info: freq 44100  format 32784 channels2  silence 0 samples 1024  padding 0 size 0 
obtainedSpec info: freq 44100  format 32784 channels2  silence 0 samples 1024  padding 0 size 4096 
init clockRate 16777216 
clockRate 16777216  fauxClock 1.000000 
available 549  len 1024
available 549  len 1024
init clockRate 16777216 
clockRate 16777216  fauxClock 1.000000 
available 668  len 1024
available 668  len 1024
init clockRate 16777216 
clockRate 16777216  fauxClock 1.000000 
available 846  len 1024
available 846  len 1024
init clockRate 16777216 
clockRate 16777216  fauxClock 1.000000 
available 738  len 1024
available 738  len 1024
00
01
21
22
23
24
25
26
desiredSpec info: freq 44100  format 32784 channels2  silence 0 samples 1024  padding 0 size 0 
obtainedSpec info: freq 44100  format 32784 channels2  silence 0 samples 1024  padding 0 size 4096 
init clockRate 16777216 
clockRate 16777216  fauxClock 1.000000 
available 737  len 1024
available 737  len 1024
init clockRate 16777216 
clockRate 16777216  fauxClock 1.000000 
available 694  len 1024
available 694  len 1024
init clockRate 16777216 
clockRate 16777216  fauxClock 1.000000 
available 447  len 1024
available 447  len 1024
init clockRate 16777216 
clockRate 16777216  fauxClock 1.000000 
available 571  len 1024
available 571  len 1024
00
01
21
22
23
24
25
26
desiredSpec info: freq 44100  format 32784 channels2  silence 0 samples 1024  padding 0 size 0 
obtainedSpec info: freq 44100  format 32784 channels2  silence 0 samples 1024  padding 0 size 4096 
init clockRate 16777216 
clockRate 16777216  fauxClock 1.000000 
available 733  len 1024
available 733  len 1024
init clockRate 16777216 
clockRate 16777216  fauxClock 1.000000 
available 765  len 1024
available 765  len 1024
init clockRate 16777216 
clockRate 16777216  fauxClock 1.000000 
available 608  len 1024
available 608  len 1024
init clockRate 16777216 
clockRate 16777216  fauxClock 1.000000 
available 533  len 1024
available 533  len 1024


00
01
21
22
23
24
25
26
desiredSpec info: freq 44100  format 32784 channels2  silence 0 samples 512  padding 0 size 0 
obtainedSpec info: freq 44100  format 32784 channels2  silence 0 samples 1024  padding 0 size 4096 
desiredSpec info: freq 44100  format 32784 channels2  silence 0 samples 1024  padding 0 size 4096 
init clockRate 16777216 
clockRate 16777216  fauxClock 1.000000 
available 349  len 1024
available 349  len 1024
init clockRate 16777216 
clockRate 16777216  fauxClock 1.000000 
available 383  len 1024
available 383  len 1024
init clockRate 16777216 
clockRate 16777216  fauxClock 1.000000 
available 689  len 1024
available 689  len 1024
init clockRate 16777216 
clockRate 16777216  fauxClock 1.000000 
available 738  len 1024
available 738  len 1024
00
01
21
22
23
24
25
26
desiredSpec info: freq 44100  format 32784 channels2  silence 0 samples 256  padding 0 size 0 
obtainedSpec info: freq 44100  format 32784 channels2  silence 0 samples 1024  padding 0 size 4096 
desiredSpec info: freq 44100  format 32784 channels2  silence 0 samples 1024  padding 0 size 4096 
init clockRate 16777216 
clockRate 16777216  fauxClock 1.000000 
available 517  len 1024
available 517  len 1024
init clockRate 16777216 
clockRate 16777216  fauxClock 1.000000 
available 516  len 1024
available 516  len 1024
init clockRate 16777216 
clockRate 16777216  fauxClock 1.000000 
available 512  len 1024
available 512  len 1024
init clockRate 16777216 
clockRate 16777216  fauxClock 1.000000 
available 516  len 1024
available 516  len 1024
00
01
21
22
23
24
25
26
desiredSpec info: freq 44100  format 32784 channels2  silence 0 samples 768  padding 0 size 0 
obtainedSpec info: freq 44100  format 32784 channels2  silence 0 samples 1536  padding 0 size 6144 
desiredSpec info: freq 44100  format 32784 channels2  silence 0 samples 1536  padding 0 size 6144 
init clockRate 16777216 
clockRate 16777216  fauxClock 1.000000 
available 861  len 1536
available 861  len 1536
init clockRate 16777216 
clockRate 16777216  fauxClock 1.000000 
available 425  len 1536
available 425  len 1536
init clockRate 16777216 
clockRate 16777216  fauxClock 1.000000 
available 872  len 1536
available 872  len 1536
init clockRate 16777216 
clockRate 16777216  fauxClock 1.000000 
available 1028  len 1536
available 1028  len 1536
00
01
21
22
23
24
25
26
desiredSpec info: freq 44100  format 32784 channels2  silence 0 samples 640  padding 0 size 0 
obtainedSpec info: freq 44100  format 32784 channels2  silence 0 samples 1280  padding 0 size 5120 
desiredSpec info: freq 44100  format 32784 channels2  silence 0 samples 1280  padding 0 size 5120 
init clockRate 16777216 
clockRate 16777216  fauxClock 1.000000 
available 888  len 1280
available 888  len 1280
init clockRate 16777216 
clockRate 16777216  fauxClock 1.000000 
available 867  len 1280
available 867  len 1280
init clockRate 16777216 
clockRate 16777216  fauxClock 1.000000 
available 501  len 1280
available 501  len 1280
init clockRate 16777216 
clockRate 16777216  fauxClock 1.000000 
available 742  len 1280
available 742  len 1280
00
01
21
22
23
24
25
26
desiredSpec info: freq 44100  format 32784 channels2  silence 0 samples 624  padding 0 size 0 
obtainedSpec info: freq 44100  format 32784 channels2  silence 0 samples 1248  padding 0 size 4992 
desiredSpec info: freq 44100  format 32784 channels2  silence 0 samples 1248  padding 0 size 4992 
init clockRate 16777216 
clockRate 16777216  fauxClock 1.000000 
available 888  len 1248
available 888  len 1248
init clockRate 16777216 
clockRate 16777216  fauxClock 1.000000 
available 883  len 1248
available 883  len 1248
init clockRate 16777216 
clockRate 16777216  fauxClock 1.000000 
available 625  len 1248
available 625  len 1248
init clockRate 16777216 
clockRate 16777216  fauxClock 1.000000 
available 808  len 1248
available 808  len 1248
00
01
21
22
23
24
25
26
desiredSpec info: freq 44100  format 32784 channels2  silence 0 samples 624  padding 0 size 0 
obtainedSpec info: freq 44100  format 32784 channels2  silence 0 samples 1248  padding 0 size 4992 
desiredSpec info: freq 44100  format 32784 channels2  silence 0 samples 1248  padding 0 size 4992 
init clockRate 16777216 
clockRate 16777216  fauxClock 1.000000 
available 441  len 1248
available 441  len 1248
init clockRate 16777216 
clockRate 16777216  fauxClock 1.000000 
available 398  len 1248
available 398  len 1248
init clockRate 16777216 
clockRate 16777216  fauxClock 1.000000 
available 786  len 1248
available 786  len 1248
init clockRate 16777216 
clockRate 16777216  fauxClock 1.000000 
available 926  len 1248
available 926  len 1248


00
01
21
22
23
24
25
26
desiredSpec info: freq 44100  format 32784 channels2  silence 0 samples 1024  padding 0 size 0 
obtainedSpec info: freq 44100  format 32784 channels2  silence 0 samples 1024  padding 0 size 4096 
init clockRate 16777216 
clockRate 16777216  fauxClock 1.000000 
available 340  len 1024
available 340  len 1024
init clockRate 16777216 
clockRate 16777216  fauxClock 1.000000 
available 781  len 1024
available 781  len 1024
init clockRate 16777216 
clockRate 16777216  fauxClock 1.000000 
available 613  len 1024
available 613  len 1024
init clockRate 16777216 
clockRate 16777216  fauxClock 1.000000 
available 592  len 1024
available 592  len 1024
00
01
21
22
23
24
25
26
desiredSpec info: freq 44100  format 32784 channels2  silence 0 samples 1024  padding 0 size 0 
obtainedSpec info: freq 44100  format 32784 channels2  silence 0 samples 1024  padding 0 size 4096 
init clockRate 16777216 
clockRate 16777216  fauxClock 1.000000 
available 349  len 1024
available 349  len 1024
init clockRate 16777216 
clockRate 16777216  fauxClock 1.000000 
available 764  len 1024
available 764  len 1024
init clockRate 16777216 
clockRate 16777216  fauxClock 1.000000 
available 786  len 1024
available 786  len 1024
init clockRate 16777216 
clockRate 16777216  fauxClock 1.000000 
available 624  len 1024
available 624  len 1024
00
01
21
22
23
24
25
26
desiredSpec info: freq 44100  format 32784 channels2  silence 0 samples 1024  padding 0 size 0 
obtainedSpec info: freq 44100  format 32784 channels2  silence 0 samples 1024  padding 0 size 4096 
init clockRate 16777216 
clockRate 16777216  fauxClock 1.000000 
available 463  len 1024
available 463  len 1024
init clockRate 16777216 
clockRate 16777216  fauxClock 1.000000 
available 710  len 1024
available 710  len 1024
init clockRate 16777216 
clockRate 16777216  fauxClock 1.000000 
available 737  len 1024
available 737  len 1024
init clockRate 16777216 
clockRate 16777216  fauxClock 1.000000 
available 376  len 1024
available 376  len 1024
00
01
21
22
23
24
25
26
desiredSpec info: freq 44100  format 32784 channels2  silence 0 samples 1024  padding 0 size 0 
obtainedSpec info: freq 44100  format 32784 channels2  silence 0 samples 1024  padding 0 size 4096 
init clockRate 16777216 
clockRate 16777216  fauxClock 1.000000 
available 549  len 1024
available 549  len 1024
init clockRate 16777216 
clockRate 16777216  fauxClock 1.000000 
available 668  len 1024
available 668  len 1024
init clockRate 16777216 
clockRate 16777216  fauxClock 1.000000 
available 846  len 1024
available 846  len 1024
init clockRate 16777216 
clockRate 16777216  fauxClock 1.000000 
available 738  len 1024
available 738  len 1024
00
01
21
22
23
24
25
26
desiredSpec info: freq 44100  format 32784 channels2  silence 0 samples 1024  padding 0 size 0 
obtainedSpec info: freq 44100  format 32784 channels2  silence 0 samples 1024  padding 0 size 4096 
init clockRate 16777216 
clockRate 16777216  fauxClock 1.000000 
available 737  len 1024
available 737  len 1024
init clockRate 16777216 
clockRate 16777216  fauxClock 1.000000 
available 694  len 1024
available 694  len 1024
init clockRate 16777216 
clockRate 16777216  fauxClock 1.000000 
available 447  len 1024
available 447  len 1024
init clockRate 16777216 
clockRate 16777216  fauxClock 1.000000 
available 571  len 1024
available 571  len 1024
00
01
21
22
23
24
25
26
desiredSpec info: freq 44100  format 32784 channels2  silence 0 samples 1024  padding 0 size 0 
obtainedSpec info: freq 44100  format 32784 channels2  silence 0 samples 1024  padding 0 size 4096 
init clockRate 16777216 
clockRate 16777216  fauxClock 1.000000 
available 733  len 1024
available 733  len 1024
init clockRate 16777216 
clockRate 16777216  fauxClock 1.000000 
available 765  len 1024
available 765  len 1024
init clockRate 16777216 
clockRate 16777216  fauxClock 1.000000 
available 608  len 1024
available 608  len 1024
init clockRate 16777216 
clockRate 16777216  fauxClock 1.000000 
available 533  len 1024
available 533  len 1024
00
01
21
22
23
24
25
26
desiredSpec info: freq 44100  format 32784 channels2  silence 0 samples 624  padding 0 size 0 
obtainedSpec info: freq 44100  format 32784 channels2  silence 0 samples 1248  padding 0 size 4992 
desiredSpec info: freq 44100  format 32784 channels2  silence 0 samples 1248  padding 0 size 4992 
init clockRate 16777216 
clockRate 16777216  fauxClock 1.000000 
available 1013  len 1248
available 1013  len 1248
init clockRate 16777216 
clockRate 16777216  fauxClock 1.000000 
available 986  len 1248
available 986  len 1248
init clockRate 16777216 
clockRate 16777216  fauxClock 1.000000 
available 1028  len 1248
available 1028  len 1248
init clockRate 16777216 
clockRate 16777216  fauxClock 1.000000 
available 947  len 1248
available 947  len 1248
00
01
21
22
23
24
25
26
desiredSpec info: freq 44100  format 32784 channels2  silence 0 samples 512  padding 0 size 0 
obtainedSpec info: freq 44100  format 32784 channels2  silence 0 samples 1024  padding 0 size 4096 
desiredSpec info: freq 44100  format 32784 channels2  silence 0 samples 1024  padding 0 size 4096 
init clockRate 16777216 
clockRate 16777216  fauxClock 1.000000 
available 512  len 1024
available 512  len 1024
init clockRate 16777216 
clockRate 16777216  fauxClock 1.000000 
available 516  len 1024
available 516  len 1024
init clockRate 16777216 
clockRate 16777216  fauxClock 1.000000 
available 512  len 1024
available 512  len 1024
init clockRate 16777216 
clockRate 16777216  fauxClock 1.000000 
available 512  len 1024
available 512  len 1024
00
01
21
22
23
24
25
26
desiredSpec info: freq 44100  format 32784 channels2  silence 0 samples 512  padding 0 size 0 
obtainedSpec info: freq 44100  format 32784 channels2  silence 0 samples 1024  padding 0 size 4096 
desiredSpec info: freq 44100  format 32784 channels2  silence 0 samples 1024  padding 0 size 4096 
init clockRate 16777216 
clockRate 16777216  fauxClock 1.000000 
available 512  len 1024
available 512  len 1024
init clockRate 16777216 
clockRate 16777216  fauxClock 1.000000 
available 512  len 1024
available 512  len 1024
init clockRate 16777216 
clockRate 16777216  fauxClock 1.000000 
available 512  len 1024
available 512  len 1024
init clockRate 16777216 
clockRate 16777216  fauxClock 1.000000 
available 512  len 1024
available 512  len 1024
00
01
21
22
23
24
25
26
desiredSpec info: freq 44100  format 32784 channels2  silence 0 samples 2048  padding 0 size 0 
obtainedSpec info: freq 44100  format 32784 channels2  silence 0 samples 2048  padding 0 size 8192 
desiredSpec info: freq 44100  format 32784 channels2  silence 0 samples 2048  padding 0 size 8192 
init clockRate 16777216 
clockRate 16777216  fauxClock 1.000000 
available 1114  len 2048
available 1114  len 2048
init clockRate 16777216 
clockRate 16777216  fauxClock 1.000000 
available 1196  len 2048
available 1196  len 2048
init clockRate 16777216 
clockRate 16777216  fauxClock 1.000000 
available 1405  len 2048
available 1405  len 2048

