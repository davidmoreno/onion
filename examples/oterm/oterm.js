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
posColumn=1
posRow=0
/// Max created row number
maxRow=0

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

/// Not used yet. FIXME.
echoLocal=false
/// Not used yet. FIXME.
cursorKeyModeApp=true
/// Just add on latest position on screen. Its the normal mode, but it can be made hard on full terminal apps (vi, for example)
dataAddEasy=true
/// Edit mode, insert (true) or orverwrite (false)
editModeInsert=false
/// If at end of line i must create a new one
autowrapMode=true
/// If the keypad is numeric
keypadNumeric=true

/// returns a substring, but the size of each character is the screen size: &nbsp; is 1 not 5
substr = function(text, i, l){
	// go to the desired char
	var op=0 // real position
	var p    // position to ret, with &..; only one char. Actually more like characters already skipped on this segment.
	for (p=0;p<i;p++){
		if (text[op]=='&'){
			while (text[op]!=';'){ if (op>text.length) return ''; op++; }
		}
		op++;
	}
	
	// Now to end
	if (!l && l!=0)
		return text.substr(op)
		
	var ip=op
	op=0
	for (p=0;p<l;p++){
		if (text[ip+op]=='&'){
			while (text[ip+op]!=';'){ if (ip+op>text.length) return ''; op++; }
		}
		op++;
	}
	return text.substr(ip,op)
}

/// Real length, changing all entities to 1 byte
elength = function(text){
	var l=0
	for (var i=0;i<text.length;i++){
		l++;
		if (text[i]=='&')
			while (text[i]!=';'){ if (i>text.length) return l; i++; }
	}
	return l
}

/// Adds a simple text to current line
addText = function(text, length){
	if (!text || text=='')
		return

	if ($('.current_line').length==0) // I need to write somewhere
		setPosition(posRow, posColumn)

	if ($('.current_line span').length==0) // I need to write somewhere
		$('.current_line').append($('<span>'))
		
	if (dataAddEasy){
		if (changedClass)
			$('.current_line').append( $('<span>').html(text).addClass(currentClass) ).showAndScroll()
		else{
			var l=$('.current_line span:last')
			l.html( $('<span>').html(l.html()+text) ).showAndScroll()
		}
	}
	else{
		var sp=getCurrentColumnSpan()
		if (!sp){
			$('.current_line').append( $('<span>').html(text).addClass(currentClass) ).showAndScroll()
			return
		}
		var p=sp[1]
		sp=sp[0]

		var tlength=elength(text)
		var t=sp.text()
		var ps // I will keep data from here on. If insert its all data, if overwrite, is from the proper position, to overwrite data.
		if (editModeInsert)
			ps=p
		else
			ps=p+tlength
		if (ps>tlength && sp.next().length!=0){
			var now=sp.text().length-p // number of overwrites
			sp.html(substr(sp.html(),0,p)+substr(text,0,now ))
			posColumn+=now
			var tt=substr(text,now)
			addText(tt, tlength-now)
			return
//			showMsg('Care here, maybe has to overwrite from next span too. for this: "'+t.substr(0,p)+text.substr(0,t.length-p)+'" for next "'+ text.substr(t.length-p) +'"')
		}
		else
			sp.html(substr(t,0,p)+text+substr(t,ps))
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

/// Parses input data and set the screen status as needed.
updateData = function(text){
	if (text=='')
		return
	//showMsg(text)
	allData+=text
	
	var clear = function(){
			length=0
			str=''
	}
	
	var flush = function(){
			addText(str,length)
			length=0
			str=''
	}
	
	var str=''
	length=0
	for (var c in text){
		c=text[c]
		if (c=='\n'){ // basic carriage return
			flush()
			column=posColumn
			newLine()
			setPosition(posRow, column)
		}
		else if (c=='\r'){
			flush()
			setPosition(posRow, 1)
		}
		else if (parserStatus==0){
			if (c=='\033'){
				parserStatus=1
				flush()
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
				flush()
				setPosition(posRow, posColumn-1)
			}
			else if (c=='\015'){
				flush()
				setPosition(posRow, 1)
			}
			else if (c=='\007'){
				flush()
				beep()
				//removeFromSOL() //
				//setPosition(posRow, 1)
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
				try{
					setStatus(str)
				}
				catch(e){
					showMsg("Exception trying to set a status!");
				}
				clear()
				parserStatus=0
			}
		}
		else if (parserStatus==3){
			if (c==';'){
				setStatusType2(str)
				clear()
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
	if (posRow>maxRow){
		dataAddEasy=true
		maxRow++;

		$('.current_line').removeClass('current_line')
		var p=$('<p>').addClass('current_line').attr('id','row_'+posRow) //.append($('<span>').addClass(currentClass))
		changedClass=false
		$('#term').append(p)
		posColumn=1
	}
	else{
		$('.current_line').removeClass('current_line')
		$('#row_'+posRow).addClass('current_line')
	}
}

/// From a status message, changes the style or status of the console, type 1: [
setStatus = function(st){
	//showMsg('Set status ['+st)
	if (!st || st=='')
		return
	if (st in specialFuncs){
		specialFuncs[st]()
	}
	else if (st.substr(-1) in specialFuncs){
		specialFuncs[st.substr(-1)]( st.substr(0,st.length-1) )
	}
	else if (ignoreType1.indexOf(st)>=0) // Just ignore.. i dont know what are they
		return
	else
		showMsg('unknown code <['+st+'>')
}

/// Sets an status of type2: ]
setStatusType2 = function(st){
	if (ignoreType2.indexOf(st)) // Just ignore.. i dont know what are they
		return
	showMsg('unknown code type2 <]'+st+'>')
}

lowModeStatus = function(st){
	if (st.charAt(0)=='?') st=st.substr(1)
	var vals=st.split(';')
	for (var i in vals){
		try{
			lowModeFuncs[vals[i]]()
		}
		catch(e){
			showMsg('Error executing low func '+vals[i])
		}
	}
}


highModeStatus = function(st){
	if (st.charAt(0)=='?') st=st.substr(1)
	var vals=st.split(';')
	for (var i in vals){
		try{
			highModeFuncs[vals[i]]()
		}
		catch(e){
			showMsg('Error executing low func '+vals[i])
		}
	}
}



/// Sets the color status
setColorStatus = function(st){
	if (!st){ // empty means reset.
		st='0'
	}
	
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
			currentAttr+=' '+classesAttr[v]
			if ('reverse'==classesAttr[v]){
				var color=currentColor
				var bg=currentBg
				if (color=='')
					currentBg='bg-white'
				else
					currentBg='bg-'+color
				if (bg=='')
					currentColor='black'
				else
					currentColor=bg.substr(3)
			}
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
	$('.current_line').removeClass('current_line')
	$('#row_'+posRow).addClass('current_line')
}

/// Removes a character, or several, from here on, passed as number + control char
removeCharsStatus = function(st){
	var n=Number(st.substr(0,st.length-1))+1
	for (i=0;i<n;i++){
		removeOneChar()
	}
}

/// Removes one char from the current line
removeOneChar = function(){
	var sp=getCurrentColumnSpan()
	var p=sp[1]
	sp=sp[0]
	
	var s=sp.text()
	sp.text(s.substr(0,p) + s.substr(p+1))
	if (sp.text()=='')
		sp.remove()
	// It is the next char looking at cursor, so no big problem about moving cursor.
}

/// Clears visible screen. Now it clears it all.
clearScreen = function(t){
	if (!t || t=='2'){
		$('#term').html($('<p id="row_1" class="current_line">'))
		maxRow=1 // I already created one
	}
	else
		showMsg('Clear screen unknown '+t)
}



/// Parses the status as given from status
setPositionStatus = function(st){
	var p
	if (!st){
		p=[1,1] 
	}
	else{
		p=st.split(';')
		p[0]=Number(p[0])
		p[1]=Number(p[1])
	}
	setPosition(p[0],p[1])
}

/// Goes to top left position
gotoTopLeft = function(){
	setPosition(1,1)
}

/// Go to a given position
setPosition = function(row,col){
	var r=$('#row_'+row)
	
	gotoRow(row)
	gotoCol(col)
	
	updateCursor()
}

/// Goes to that row, ensuring it exists
gotoRow = function(rn){
	$('.current_line').removeClass('current_line')
	dataAddEasy=false
	/*
	alert(maxRow)
	alert(rn)
	*/
	while (maxRow<rn){
		//showMsg('Adding line to get to row '+rn)
		newLine()
	}
	var r=$('#row_'+rn)
	r.addClass('current_line')
	posRow=rn
}


/// Goes to the given column
gotoCol = function(cn){
	var l=elength($('.current_line').text())+1
	if (cn>l){
		
		var s=''
		for (var i=l;i<cn;i++) s+='&nbsp;';
		addText(s,s.length)
	}
	dataAddEasy=(cn==l)
		
	posColumn=cn
}

/// visual beep
beep = function(){
	$('#cursor').css('background','yellow')
	setTimeout(function(){ $('#cursor').css('background','') }, 500)
	//posRow++
}

/// Hides the cursor
hideCursor = function(){
	$('span#cursor').css('background','red').css('opacity','0.8')
}

/// shows the cursor
showCursor = function(){
	$('span#cursor').css('background','white').css('opacity','0.8')
}

/// Activates remote echo; no local echo
remoteEcho = function(){
	echoLocal=false
}

/// Activates remote echo; no local echo
localEcho = function(){
	echoLocal=true
}


/// Activates remote echo; no local echo
cursorKeyModeApplication = function(){
	cursorKeyModeApp=true
}

/// Activates remote echo; no local echo
cursorKeyModeCursor = function(){
	cursorKeyModeApp=false
}

/// Moves left some positions
scrollLeft = function(st){
	var l
	if (!st)
		l=1
	else
		l=Number(st)
	setPosition(posRow, posColumn-l)
}

/// Moves left some positions
scrollRight = function(st){
	var r
	if (!st)
		r=1
	else
		r=Number(st)
	setPosition(posRow, posColumn+r)
}

/// Sets the vertical position
setVerticalPosition = function(st){
	setPosition(Number(st), posColumn)
}

/// Sets the vertical position
setHorizontalPosition = function(st){
	setPosition(posRow, Number(st))
}


overwriteMode = function(){
	editModeInsert=false
}

insertMode = function(){
	editModeInsert=true
}

autowrapOn = function(){
	autowrapMode=true
}

autowrapOff = function(){
	autowrapMode=false
}

keyPadModeNumeric = function(){
	keypadNumeric=true
}

keyPadModeApplication = function(){
	keypadNumeric=false
}

/// Removes from the cursor position to the end of the line.
removeToEOL = function(){
	var sn=getCurrentColumnSpan()
	if (!sn)
		return
	var span=sn[0]
	var h=span.text() // it can be text, and not html, as all the &amp; will be translated, so we have proper size.
	span.text(h.substr(0, sn[1]))
	span=span.next()
	while(span.length!=0){
		span.remove()
		span=span.next()
	}
}

/// Removes all line from start of line to current position. Current position is moved.
removeFromSOL = function(){
	var sn=getCurrentColumnSpan()
	if (!sn){
		$('.current_line span').remove()
		return
	}
	var span=sn[0]
	var h=span.text() // it can be text, and not html, as all the &amp; will be translated, so we have proper size.
	span.text(h.substr(sn[1]))
	span=span.prev()
	while(span.length!=0){
		span.remove()
		span=span.prev()
	}
}

/**
 * @return returns the current span and position inside span.
 */
getCurrentColumnSpan = function(){
	var i=0
	var m=posColumn
	var ret=false
	$('.current_line > span').each(function(){
		if (ret!=false)
			return
		var t=$(this)
		var l=t.text().length
		if (l>(m-i-1)){
			ret=[t, m-i-1]
			return
		}
		i+=l
	})
	return ret
}

showMsg = function(msg){
	$('#msg').fadeIn()
	var d=$('<div>').text(msg).fadeIn()
	
	var hideIt = function(){ 
		d.fadeOut(function(){ 
			$(this).remove() 
			if ($('#msg div').length==0)
				$('#msg').fadeOut()
		})
	}
	
	setTimeout(hideIt, 10000)
	d.click(hideIt)

	$('#msg').append(d)
}

/// Calcualtes current terminal size, in characters, [ rows, cols ]
getSize = function(){
}

updateGeometry = function(){
	var t=$(document)
	nrRows = t.height()/charHeight
	nrCols = t.width()/charWidth
	showMsg('New geometry is '+nrRows+' rows, '+nrCols+' columns.')
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
	
	$('#msg').fadeOut().html('')
	showMsg('Ready')
	
	updateGeometry()
	
	$(window).resize(updateGeometry)
	
	
	$.fn.extend({
		showAndScroll: function() {
					try{ // just try, no big deal if can not.
						window.scrollTo(0, $(this).position()['top']);
					}
					catch(e){
					}
				}
	});

})
