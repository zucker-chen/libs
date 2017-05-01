/*
 * base64.h
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

#ifndef _12345_BASE64_H_54321_
#define _12345_BASE64_H_54321_

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Note: The 'output' variable should be a preallocated buffer that
 * should have enough allocated space for the encoded/decoded
 * result. Failure to do so will result in a buffer overrun.
 */

/*
 * input:   input data
 * len:     input data length and return output data length
 * output:  output data, the buffer space must be >= (3*(len/3+((len%3) > 0)))*4/3+1 ¡Ö(len+3)*4/3+1  ==> +1 = end char '\0'
 */
void base64_encode(unsigned char *input, unsigned int *len, unsigned char *output);

/*
 * input:   input data
 * len:     input data length and return output data length
 * output:  output data, the buffer space must be >= (len*3)/4+1  ==> +1 = end char '\0'
 */
void base64_decode(unsigned char *input, unsigned int *len, unsigned char *output);


#ifdef __cplusplus
}
#endif

#endif
