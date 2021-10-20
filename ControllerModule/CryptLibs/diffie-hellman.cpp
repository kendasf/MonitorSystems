//
// diffie-hellman.c
//
#include "bigdigits.h"
#include "diffie-hellman.h"

#include <stdlib.h>
#include <string.h>


//
// void DiffieHellman_GetKeys
//
// Creates the shared secret key and PublicKeyB from G, P and PublicKeyA parameters received
// from the peer. Diffie-Hellman key exchange algorithm is used. ALl keys are 256-bit long.
//
void DiffieHellman_GetKeys(unsigned char G[],
						   unsigned char P[],
						   unsigned char publicKeyA[],
						   unsigned char sharedSecret[],
						   unsigned char publicKeyB[])
{
	int i;
	DIGIT_T mpG[8];
	DIGIT_T mpP[8];
	DIGIT_T mpPublicKeyA[8];
	DIGIT_T mpSharedSecret[8];
	DIGIT_T mpPublicKeyB[8];
	DIGIT_T mpLocalSecret[8];

	//
	// convert in parameters to DIGIT_T type
	//
	for(i = 0; i < 8; i++)
	{
		mpP[7-i] = (P[i*4] << 24) | (P[i*4+1] << 16) | (P[i*4+2] << 8) | (P[i*4+3]);
		mpPublicKeyA[7-i] = (publicKeyA[i*4] << 24) | (publicKeyA[i*4+1] << 16) | (publicKeyA[i*4+2] << 8) | (publicKeyA[i*4+3]);
	}

	memset(mpG, 0, 8 * sizeof(DIGIT_T));
	mpG[0] = G[0] & 0x000000FF;

	//
	// create the local secret
	//
	for(i = 0; i < 8; i++)
	{
		mpLocalSecret[i] = rand();
	}

	//
	// calculate the shared secret symmetric key
	//
	mpModExp(mpSharedSecret, mpPublicKeyA, mpLocalSecret, mpP, 8);

	//
	// calculate the publicKeyB
	//
	mpModExp(mpPublicKeyB, mpG, mpLocalSecret, mpP, 8);

	//
	// convert DIGIT_T type to output parameters
	//
	for(i = 0; i < 32; i++)
	{
		sharedSecret[i] = mpSharedSecret[7-i/4] >> (24 - (i%4)*8);
		publicKeyB[i] = mpPublicKeyB[7-i/4] >> (24 - (i%4)*8);
	}

	// and that's it...
}
