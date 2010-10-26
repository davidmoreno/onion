/*
	Onion HTTP server library
	Copyright (C) 2010 David Moreno Montero

	This program is free software: you can redistribute it and/or modify
	it under the terms of the GNU Affero General Public License as
	published by the Free Software Foundation, either version 3 of the
	License, or (at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU Affero General Public License for more details.

	You should have received a copy of the GNU Affero General Public License
	along with this program.  If not, see <http://www.gnu.org/licenses/>.
	*/

#include <string.h>
#include <malloc.h>

/// Decode table. Its the inverse of the code table. (cb64).
static const char db64[]={ // 16 bytes each line, 8 lines. Only 128 registers
	255,255,255,255, 255,255,255,255, 255,255,255,255, 255,255,255,255,  // 16
	255,255,255,255, 255,255,255,255, 255,255,255,255, 255,255,255,255,  // 32
	255,255,255,255, 255,255,255,255, 255,255,255,62,  255,255,255,63,   // 48
	52,53,54,55,     56,57,58,59,     60,61,255,255,   255,255,255,255,  // 64
	255,0,1,2,       3,4,5,6,         7,8,9,10,        11,12,13,14,      // 80
	15,16,17,18,     19,20,21,22,     23,24,25,255,    255,255,255,255,  // 96
	255,26,27,28,    29,30,31,32,     33,34,35,36,     37,38,39,40,      // 112
	41,42,43,44,     45,46,47,48,     49,50,51,255,    255,255,255,255   // 128
};

/// Coding table. 
static const char cb64[]="ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

void printf_bin(const char c, int n){
	int i;
	//fprintf(stderr, "%c %-3d ", c,c);
	for(i=n-1;i>=0;i--){
		fprintf(stderr, "%c", ((c>>i)&1) ? '1' : '0');
	}
	fprintf(stderr, " ");
}

/**
 * @short Decodes a base64 into a new char* (must be freed later).
 *
 * At length, if not NULL we set the final length of the decoded base.
 *
 * The buffer might be sligthly bigger (up to 3 bytes), but length is always right.
 */
char *onion_base64_decode(const char *orig, int *length){
	int ol=strlen(orig)-1;
	int l=ol*3/4;
	char *ret=malloc(l+1);
	
	if (ol<4){ // Not even a block
		if (length)
			*length=0;
		ret[0]=0;
		return ret;
	}

	
	int i,j;
	for (i=0,j=0;i<ol;i+=4,j+=3){
		unsigned char o[4],c;
		int k;
		for (k=0;k<4;k++){
			while ( (i+k)<ol && ((c=db64[(int)orig[i+k]]) & 192) ) i++;
			o[k]=c;
		}
		
		ret[j]  =((o[0]&0x3F)<<2)+((o[1]&0x30)>>4);
		ret[j+1]=((o[1]&0x0F)<<4)+((o[2]&0x3C)>>2);
		ret[j+2]=((o[2]&0x03)<<6)+(o[3]);
	}
	if (length){ // Set the right size.. only if i need it.
		*length=j;
		//fprintf(stderr, "ol-2 %d, length %d\n",ol-2,j);
		if (orig[ol-2]=='='){
			*length=j-2;
		}
		else if (orig[ol-1]=='='){
			*length=j-1;
		}
			
		ret[*length]=0;
	}
	
	return ret;
}


/**
 * @short Encodes a byte array to a base64 into a new char* (must be freed later).
 */
char *onion_base64_encode(const char *orig, int length){
	/// that chars + \n one per every 57 + \n\0 at end, and maybe two '='
	char *ret=malloc(((length*4)/3) + (length/57) + 2 + 3 );
	if (length==0){ // easy case.. here.
		ret[0]='\0';
		return ret;
	}
	
	int i,j, I,J;
	// First many lines,  I hope the compiler unloops properly...
	for (I=0,J=0;I<length-57;I+=57,J+=77){ // 57 chars are 76 chars in base64. Just a place to set the new line.
		const char *o=&orig[I];
		char *r=&ret[J];
		for (i=0,j=0;i<57;i+=3,j+=4){
			char a=o[i];
			char b=o[i+1];
			char c=o[i+2];
			
			r[j]=(a>>2)&0x3F;
			r[j+1]=((a<<4)&0x30) + ((b>>4)&0x0F);
			r[j+2]=((b<<2)&0x3C) + ((c>>6)&0x03);
			r[j+3]=(c&0x3F);
		}
		for(i=0;i<76;i++){
				r[i]=cb64[(int)r[i]];
		}
		r[j]='\n';
		r[j+1]='\0';
	}
	// Now the same, with last lines
	{
		const char *o=&orig[I];
		char *r=&ret[J];
		int maxel=length-I; // max at end loop
		for (i=0,j=0;i<maxel-3;i+=3,j+=4){
			char a=o[i];
			char b=o[i+1];
			char c=o[i+2];
			
			r[j]=(a>>2)&0x3F;
			r[j+1]=((a<<4)&0x30) + ((b>>4)&0x0F);
			r[j+2]=((b<<2)&0x3C) + ((c>>6)&0x03);
			r[j+3]=(c&0x3F);
		}
		//fprintf(stderr,"i %d maxel %d\n",i,maxel);
		{ // last 3 chars
			char a=(i<maxel) ? o[i] : 0;
			char b=((i+1)<maxel) ? o[i+1] : 0;
			char c=((i+2)<maxel) ? o[i+2] : 0;
			
			r[j]=(a>>2)&0x3F;
			r[j+1]=((a<<4)&0x30) + ((b>>4)&0x0F);
			r[j+2]=((b<<2)&0x3C) + ((c>>6)&0x03);
			r[j+3]=(c&0x3F);
/*
			printf_bin(a,8);
			printf_bin(b,8);
			printf_bin(c,8);
			fprintf(stderr," -> ");
			printf_bin(r[j],6);
			printf_bin(r[j+1],6);
			printf_bin(r[j+2],6);
			printf_bin(r[j+3],6);
			fprintf(stderr,"\n");
			*/
		}
		for(i=0;i<j+4;i++){
				r[i]=cb64[(int)r[i]];
		}
		j+=4;
		if ((length%3)==1){
			r[j-1]='=';
			r[j-2]='=';
		}
		if ((length%3)==2){
			r[j-1]='=';
		}
		r[j++]='\n';
		r[j]='\0';
	}
	return ret;
}
