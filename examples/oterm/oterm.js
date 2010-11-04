/*
	Onion HTTP terminal
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

/**
 * On this file lies all the parsing of terminal data, to make it work on a HTML.
 */

/// As the state machines changes and finds [0..m it can change the class.
currentAttr=''
currentBg=''
currentColor=''
currentClass='' // just the cached sum of all before here.
/// Current line position
posColumn=0
posRow=0
/// Initial timeout and default
defaultTimeout=100
timeout=defaultTimeout
/// To cancel a long timeout when new chars are sent to make it a fast timeout
timeoutId=0 
/// Timeout to show a message
msgTimeout=0
/// Status of the parser: 0 normal, 1 set status type 1, 2 set status type 2.
parserStatus=0

/// All data readen so far. For debugging. FIXME
allData=''

/// Stupid guess, changed at load time.
charWidth=8
charHeight=8

/// Set to true whenclass changed and need a new span. Else just use latest span to add text.
changedClass=false

/// Parses input data and set the screen status as needed.
updateData = function(text){
	allData+=text
	var str=''
	length=0
	for (c in text){
		c=text[c]
		if (c=='\n'){ // basic carriage return
			addText(str,length)
			length=0
			str=''
			newLine()
		}
		else if (parserStatus==0){
			if (c=='\033'){
				parserStatus=1
				addText(str)
				str=''
				length=0
			}
			else if (c=='\t'){
				var n=8-(posColumn%8)
				for (var i=0;i<n;i++){
					str+='&nbsp;'
					length++
				}
			}
			else if (c==' '){
				length++
				str+='&nbsp;'
			}
			else if (c=='&'){
				length++
				str+='&and;'
			}
			else if (c=='<'){
				length++
				str+='&lt;'
			}
			else if (c=='>'){
				length++
				str+='&gt;'
			}
			else if (c=='\010'){
				posColumn--
				updateCursor()
			}
			else if (c=='\007'){
				//beep()
			}
			else{
				length++
				str+=c
			}
		}
		else if (parserStatus==1){
			if (c=='['){
				parserStatus=2
			}
			else if (c==']'){
				parserStatus=3
			}
		}
		else if (parserStatus==2){
			str+=c
			if ('0123456789;?'.indexOf(c)<0){
				setStatus(str)
				str=''
				parserStatus=0
			}
		}
		else if (parserStatus==3){
			if (c==';'){
				setStatusType2(str)
				str=''
				parserStatus=0
			}
			else
				str+=c
		}
	}
	addText(str,length)
}


newLine = function(){
	posRow++;
	$('.current_line').removeClass('current_line')
	var p=$('<p>').addClass('current_line').attr('id','row_'+posRow).append($('<span>').addClass(currentClass))
	changedClass=false
	$('#term').append(p)
	posColumn=1
}

/// From a status message, changes the style or status of the console, type 1: [
setStatus = function(st){
	if (!st || st=='')
		return
	if (st in specialFuncs){
		specialFuncs[st]()
	}
	else if (st.substr(-1)=='m'){
		setColorStatus(st.substr(0,st.length-1))
	}
	else if (st.substr(-1)=='H'){
		var p=st.substr(0,st.length-1).split(';')
		setPosition(Number(p[0]),Number(p[1]))
	}
	else if (st.substr(-1)=='P'){
		var n=Number(st.substr(0,st.length-1))+1
		for (i=0;i<n;i++){
			removeOneChar()
		}
	}
	else if (ignoreType1.indexOf(st)>=0) // Just ignore.. i dont know what are they
		return
	else
		showMsg('unknown code <'+st+'>')
}

/// Sets an status of type2: ]
setStatusType2 = function(st){
	if (ignoreType2.indexOf(st)) // Just ignore.. i dont know what are they
		return
	showMsg('unknown code type2 <'+st+'>')
}


/// Sets the color status
setColorStatus = function(st){
	var l=st.split(';')
	for (var v in l){
		v=Number(l[v])
		if (v==0){
			currentAttr=''
			currentBg=''
			currentColor=''
		}
		else if (v in classesColor){
			currentColor=classesColor[v]
		}
		else if (v in classesAttr){
			currentAttr=classesAttr[v]
		}
		else if (v in classesBg){
			currentBg=classesBg[v]
		}
		else
			showMsg('unknown color code '+v)
	}
	changedClass=true
	currentClass=currentAttr+' '+currentBg+' '+currentColor;
}

/// Places the cursor on its place
updateCursor = function(){
	$('span#cursor').css('top',1+(posRow-1)*charHeight).css('left',(posColumn-1)*charWidth)
}

/// Removes one char from the current line
removeOneChar = function(){
	var sp=$('.current_line span:last')
	var s=sp.html()
	sp.html(s.substr(0,s.length-1))
	if (sp.html()=='')
		sp.remove()
	posColumn--;
	if (posColumn<0)
		posColumn=0
	updateCursor()
}

/// Clears visible screen. Now it clears it all.
clearScreen = function(){
	$('#term').html('')
	posRow=0
	newLine()
}

/// Goes to top left position
gotoTopLeft = function(){
	setPosition(1,1)
}

/// Go to a given position
setPosition = function(row,col){
	var r=$('#row_'+row)
	/*
	if (r.length==0){
		$('')
	}
	/*/
	posRow=row
	posCol=col
	updateCursor()
}


/// Adds a simple text to current line
addText = function(text, length){
	if (!text || text=='')
		return
	if (changedClass)
		$('.current_line').append( $('<span>').html(text).addClass(currentClass) ).showAndScroll()
	else{
		var l=$('.current_line span:last')
		l.html( $('<span>').html(l.html()+text) ).showAndScroll()
	}
	posColumn+=length
	
	updateCursor()
}

cacheSendKeys=''
onpetition=false // no two petitions at the same time.

/**
 * @short Request new data, and do the timeout for next petition
 *
 * Data send is serialized: only one peittion can be on the air. If a new petition is done, then 
 * it must wait until the reponse of latest arrives, and then make a new one. This helps a lot the interactivity, and solves
 * serialization problems.
 */
requestNewData = function(keyvalue){
	if (keyvalue)
		cacheSendKeys+=keyvalue
	if (onpetition){
		return
	}
	
	onpetition=true
	if (cacheSendKeys!=''){
		$.get('/term',{type:cacheSendKeys}, updateDataTimeout,'plain')
		cacheSendKeys=''
	}
	else
		$.get('/term',updateDataTimeout,'plain')
}

/// Sets the result of data load, and set a new timeout for later. If there is data to send, do it now.
updateDataTimeout = function(text){
	clearTimeout(timeoutId)
	updateData(text)
	
	onpetition=false
	if (cacheSendKeys!='')
		requestNewData()
	else{
		/// Update timeout times, incremental as important things use to be at the same times, and then big times of nothing
		if (text && text!='')
			timeout=defaultTimeout
		else{
			if (timeout<=5000)
				timeout=timeout*2.5
		}

		timeoutId=setTimeout(requestNewData, timeout)
	}
}

showHex = function(){
	var s=''
	for (var i in allData){
		var v=allData.charCodeAt(i)
		s+=allData[i]+" 0"+v.toString(8)+"\n"
	}
	return s
}

/// visual beep
beep = function(){
	$('body').css('background','yellow')
	setTimeout(function(){ $('body').css('background','') }, 200)
	posRow++
}


showMsg = function(msg){
	var d=$('<div>').text(msg).fadeIn()
	setTimeout(function(){ d.fadeOut(function(){ $(this).remove() }) }, 10000)

	$('#msg').append(d)
}

/// Prepares basic status of the document: keydown are processed, and starts the updatedata.
$(document).ready(function(){
	var t=$('#term span')
	charWidth=(t.width()/t.text().length)
	charHeight=t.height()-3
	
	updateCursor()
	
	$('#term').html('')
	newLine()
	requestNewData()
	
	$('#msg').html('')
	
	$.fn.extend({
		showAndScroll: function() {/*
				$(this).show();
				if( $(this).position()['top'] )*/
					window.scrollTo(0, $(this).position()['top']);
				}
	});

})
