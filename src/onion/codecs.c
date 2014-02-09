/*
	Onion HTTP server library
	Copyright (C) 2010-2014 David Moreno Montero and othes

	This library is free software; you can redistribute it and/or
	modify it under the terms of, at your choice:
	
	a. the Apache License Version 2.0. 
	
	b. the GNU General Public License as published by the 
		Free Software Foundation; either version 2.0 of the License, 
		or (at your option) any later version.
	 
	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of both libraries, if not see 
	<http://www.gnu.org/licenses/> and 
	<http://www.apache.org/licenses/LICENSE-2.0>.
	*/

#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdio.h>
#ifdef HAVE_GNUTLS
#include <gnutls/gnutls.h>
#include <gnutls/crypto.h>
#endif

#include "log.h"
#include "codecs.h"

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
	if (orig==NULL)
		return NULL;
	int ol=strlen(orig)-1;
	while ((ol>=3) && (orig[ol]=='\n' || orig[ol]=='\r' || orig[ol]=='\0')) ol--; // go to real end, and set to next. Order of the && is important.
	ol++;
	
	//fprintf(stderr,"ol %d\n",ol);
	int l=ol*3/4;
	char *ret=malloc(l+1);
	
	if (ol<4){ // Not even a block
		if (length)
			*length=0;
		ret[0]=0;
		return ret;
	}

	
	int i,j;
	char c=0; // Always used properly, but gcc -O2 -Wall thinks it may get used not initialized.
	for (i=0,j=0;i<ol;i+=4,j+=3){
		unsigned char o[4];
		int k;
		for (k=0;k<4;k++){
			while ( (i+k)<ol && ((c=db64[(int)orig[i+k]]) & 192) ) i++;
			//fprintf(stderr,"%c ",orig[i+k]);
			o[k]=c;
		}
		
		ret[j]  =((o[0]&0x3F)<<2)+((o[1]&0x30)>>4);
		ret[j+1]=((o[1]&0x0F)<<4)+((o[2]&0x3C)>>2);
		ret[j+2]=((o[2]&0x03)<<6)+(o[3]);
		/*
		printf_bin(o[0], 6);
		printf_bin(o[1], 6);
		printf_bin(o[2], 6);
		printf_bin(o[3], 6);
		fprintf(stderr," -> ");
		printf_bin(ret[j], 8);
		printf_bin(ret[j+1], 8);
		printf_bin(ret[j+2], 8);
		fprintf(stderr,"\n");
		*/
	}
	if (length){ // Set the right size.. only if i need it.
		*length=j;
		//fprintf(stderr, "ol-2 %d, length %d\n",ol-2,j);
		if (orig[ol-2]=='=')
			*length=j-2;
		else if (orig[ol-1]=='=')
			*length=j-1;
	}
	if (orig[ol-1]=='=')
		ret[j-1]=0;
	/*
	else if (orig[ol]=='=')
		ret[j]=0;*/
	else
		ret[j]=0;
	
	return ret;
}


/**
 * @short Encodes a byte array to a base64 into a new char* (must be freed later).
 */
char *onion_base64_encode(const char *orig, int length){
	if (orig==NULL)
		return NULL;
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


/**
 * @short Performs unquote inplace.
 *
 * It can be inplace as char position is always at least in the same on the destination than in the origin
 */
void onion_unquote_inplace(char *str){
	char *r=str;
	char *w=str;
	char tmp[3]={0,0,0};
	while (*r){
		if (*r == '%'){
			r++;
			tmp[0]=*r++;
			tmp[1]=*r;
			*w=strtol(tmp, (char **)NULL, 16);
		}
		else if (*r=='+'){
			*w=' ';
		}
		else{
			*w=*r;
		}
		r++;
		w++;
	}
	*w='\0';
}

/**
 * @short Performs URL quoting, memory is allocated and has to be freed.
 *
 * As size necesary is dificult to measure, it first check how many should be encoded, and on a second round it encodes it.
 */
char *onion_quote_new(const char *str){
	int i;
	int l=strlen(str);
	int nl=1;
	for (i=0;i<l;i++){
		if (!isalnum(str[i]))
			nl+=3;
		else
			nl++;
	}
	// Now do the quotation part
	char *ret=malloc(nl);
	onion_quote(str, ret, nl);
	return ret;
}


/// Performs URL quoting, uses auxiliary res, with maxlength size. If more, do up to where I can, and cut it with \0.
int onion_quote(const char *str, char *res, int maxlength){
	static char hex[]="0123456789ABCDEF";
	int nl=0;
	int l=strlen(str);
	int i;
	for (i=0;i<l;i++){
		if (i>=maxlength)
			break;
		char c=str[i];
		if (isalnum(c))
			res[nl++]=c;
		else{
			if (i+2>=maxlength)
				break;

			res[nl++]='%';
			res[nl++]=hex[(c>>4)&0x0F];
			res[nl++]=hex[c&0x0F];
		}
	}
	res[nl]=0;
	return nl;
}


/// Performs C quotation: changes " for \". Usefull when sending data to be interpreted as JSON. Returned data must be freed.
char *onion_c_quote_new(const char *str){
	char *ret;
	int l=3; // The quotes + \0
	const unsigned char *p=(const unsigned char *)str;
	while( *p != '\0'){ 
		if (*p=='\r' || *p=='"' || *p=='\\' || *p=='\t')
			l++;
		else if (*p=='\n')
			l+=4; // \n"[real newline]"
		else if(*p>127)
			l+=4;
		l++; p++;
	}
	ret=malloc(l);
	onion_c_quote(str, ret, l);
	return ret;
}

/// Performs the C quotation on the ret str. Max length is l.
char *onion_c_quote(const char *str, char *ret, int l){
	const unsigned char *p=(const unsigned char *)str;
	char *r=ret;
	*r++='"';
	l-=3; // both " at start and end, and \0
	while( *p != '\0'){ 
		if (*p=='\n'){ 
			*r='\\'; 
			r++; 
			*r='n'; 
			r++; 
			*r='"'; 
			r++; 
			*r='\n'; 
			r++; 
			*r='"'; 
		} 
		else if (*p=='\r'){
			*r='\\'; 
			r++; 
			*r='n'; 
		}
		else if (*p=='"'){
			*r='\\'; 
			r++; 
			*r='"'; 
		}
		else if (*p=='\\'){
			*r='\\'; 
			r++; 
			*r='\\'; 
		}
		else if (*p=='\t'){
			*r='\\'; 
			r++; 
			*r='t'; 
		}
		else if (*p>127){
			int c=*p;
			if (l<4){ // does not fit!
				*r='\0';
				return ret;
			}
			*r='\\';
			r++;
			*r='0'+((c>>6)&0x03);
			r++;
			*r='0'+((c>>3)&0x07);
			r++;
			*r='0'+(c&0x07);
		}
		else{
			*r=*p;
		}
		r++; p++; 
		l--;
		if (l<=0){
			*r='\0';
			break;
		}
	}
	*r++='"';
	*r++='\0';
	
	return ret;
}

/**
 * @short Calculates the SHA1 checksum of a given data
 */
void onion_sha1(const char *data, int length, char *result){
#ifndef HAVE_GNUTLS
	ONION_ERROR("Cant calculate SHA1 if gnutls is not compiled in! Aborting now");
	exit(1);
#else
	gnutls_hash_fast(GNUTLS_DIG_SHA1, data, length, result); 
#endif
}

/// At p inserts the proper encoding of c, and returns the new string cursor (end of inserted symbols).
char *onion_html_add_enc(char c, char *p){
	switch(c){
		case '<':
			*p++='&';
			*p++='l';
			*p++='t';
			*p++=';';
			break;
		case '>':
			*p++='&';
			*p++='g';
			*p++='t';
			*p++=';';
			break;
		case '"':
			*p++='&';
			*p++='q';
			*p++='u';
			*p++='o';
			*p++='t';
			*p++=';';
			break;
		case '\'':
			*p++='&';
			*p++='#';
			*p++='3';
			*p++='9';
			*p++=';';
			break;
		default:
			*p++=c;
	}
	return p;
}

/// Returns the encoding for minimal set
int onion_html_encoding_size(char c){
	switch(c){
		case '<':
			return 4;
		case '>':
			return 4;
		case '"':
			return 6;
		case '\'':
			return 5;
	}
	return 1;
}

/**
 * @short Calculates the HTML encoding of a given string
 * 
 * If needs encoding returns a new string that should be deallocated, if not, returns NULL.
 */
char *onion_html_quote(const char *str){
	/// first calculate size
	int size=0;
	const char *p=str;
	while( (*p) ){
		size+=onion_html_encoding_size(*p);
		p++;
	}
	if (size==(p-str))
		return NULL;
	char *ret=calloc(1,size+1);
	p=str;
	char *t=ret;
	while( (*p) ){
		t=onion_html_add_enc(*p, t);
		p++;
	}
	*t='\0';
	return ret;
}

