/*
 * base64.c
 *
 * Created by Sean Starkey <sean@seanstarkey.com>
 *
 * NO COPYRIGHT - THIS IS 100% IN THE PUBLIC DOMAIN
 *
 * The original version is available at:
 *    https://github.com/SeanStarkey/base64
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR(S) AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR(S) OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include "base64.h"
#include <stdio.h>

const unsigned char encodeCharacterTable[65] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
const unsigned char decodeCharacterTable[256] = {
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1
	,-1,62,-1,-1,-1,63,52,53,54,55,56,57,58,59,60,61,-1,-1,-1,-1,-1,-1,-1,0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21
	,22,23,24,25,-1,-1,-1,-1,-1,-1,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1
	,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1
};


/*
 * input:   input data
 * len:     input data length and return output data length
 * output:  output data, the buffer space must be >= (3*(len/3+((len%3) > 0)))*4/3+1 ¡Ö(len+3)*4/3+1  ==> +1 = end char '\0'
 */
void base64_encode(unsigned char *input, unsigned char *output, int *len)
{
	char buff1[3];
	unsigned char buff2[4];
	unsigned char i=0, j;
	unsigned int input_cnt=0;
	unsigned int output_cnt=0;
    int input_length = *len;

	while(input_cnt<input_length)
	{
		buff1[i++] = input[input_cnt++];
		if (i==3)
		{
			output[output_cnt++] = encodeCharacterTable[(buff1[0] & 0xfc) >> 2];
			output[output_cnt++] = encodeCharacterTable[((buff1[0] & 0x03) << 4) + ((buff1[1] & 0xf0) >> 4)];
			output[output_cnt++] = encodeCharacterTable[((buff1[1] & 0x0f) << 2) + ((buff1[2] & 0xc0) >> 6)];
			output[output_cnt++] = encodeCharacterTable[buff1[2] & 0x3f];
			i=0;
		}
	}
	if (i)
	{
		for(j=i;j<3;j++)
		{
			buff1[j] = '\0';
		}
		buff2[0] = (buff1[0] & 0xfc) >> 2;
		buff2[1] = ((buff1[0] & 0x03) << 4) + ((buff1[1] & 0xf0) >> 4);
		buff2[2] = ((buff1[1] & 0x0f) << 2) + ((buff1[2] & 0xc0) >> 6);
		buff2[3] = buff1[2] & 0x3f;
		for (j=0;j<(i+1);j++)
		{
			output[output_cnt++] = encodeCharacterTable[buff2[j]];
		}
		while(i++<3)
		{
			output[output_cnt++] = '=';
		}
	}
	output[output_cnt] = 0;
    *len = output_cnt;
}


/*
 * input:   input data
 * len:     input data length and return output data length
 * output:  output data, the buffer space must be >= (len*3)/4+1  ==> +1 = end char '\0'
 */
void base64_decode(unsigned char *input, unsigned char *output, int *len)
{
	unsigned char buff1[4];
	unsigned char buff2[4];
	unsigned char i=0, j;
	unsigned int input_cnt=0;
	unsigned int output_cnt=0;
    int input_length = *len;

	while(input_cnt<input_length)
	{
		buff2[i] = input[input_cnt++];
		if (buff2[i] == '=')
		{
			break;
		}
		if (++i==4)
		{
			for (i=0;i!=4;i++)
			{
				buff2[i] = decodeCharacterTable[buff2[i]];
			}
			output[output_cnt++] = (char)((buff2[0] << 2) + ((buff2[1] & 0x30) >> 4));
			output[output_cnt++] = (char)(((buff2[1] & 0xf) << 4) + ((buff2[2] & 0x3c) >> 2));
			output[output_cnt++] = (char)(((buff2[2] & 0x3) << 6) + buff2[3]);
			i=0;
		}
	}
	if (i)
	{
		for (j=i;j<4;j++)
		{
			buff2[j] = '\0';
		}
		for (j=0;j<4;j++)
		{
			buff2[j] = decodeCharacterTable[buff2[j]];
		}
		buff1[0] = (buff2[0] << 2) + ((buff2[1] & 0x30) >> 4);
		buff1[1] = ((buff2[1] & 0xf) << 4) + ((buff2[2] & 0x3c) >> 2);
		buff1[2] = ((buff2[2] & 0x3) << 6) + buff2[3];
		for (j=0;j<(i-1); j++)
		{
			output[output_cnt++] = (char)buff1[j];
		}
	}
	output[output_cnt] = 0;
    *len = output_cnt;
}
