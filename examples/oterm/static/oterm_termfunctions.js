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
 * On this file lie the functions needed to make the terminal respond to the key codes.
 * 
 * Here the parsing is not done, just he functions that modify current terminal status
 */

/// Moves to the next line.
newLine = function(){
	posRow++;
	setPosition(posRow, posColumn)
}

/// From a status message, changes the style or status of the console, type 1: [
setStatus = function(st){
	//showMsg('Set status ['+st)
	if (!st || st=='')
		return
		
	statusLog.push(st)
		
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
	//showMsg('Set status <]'+st+'>')
	if (st in specialFuncs2){
		specialFuncs2[st]()
	}
	else if (ignoreType2.indexOf(st)) // Just ignore.. i dont know what are they
		return
	else
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
	var nr=$('#row_'+posRow)
	if (!nr.hasClass('current_line')){
		current_line.removeClass('current_line')
		nr.addClass('current_line')
		current_line=nr
	}

	var posY
	if (current_line.length!=0)
		posY=current_line.position().top
	else
		posY=1+(posRow-1)*charHeight

	$('span#cursor').css('top',posY).css('left',(posColumn-1)*charWidth)

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
	if (!t || t=='0'){ // from cursor to end of screen
		var sp=getCurrentColumnSpan()
		if (!sp)
			return
		sp[0].html(substr(sp[0].html(), 0, sp[1])) // on current span, remove leading text
		
		// leading spans
		var n=sp[0].next()
		while (n.length!=0){
			var nn=n.next()
			n.remove()
			n=nn
		}
		// leading lines
		var n=current_line.next()
		while (n.length!=0){
			var nn=n.next()
			n.remove()
			n=nn
		}
		var l=$('#term p:last')
		if (l.length==0){
			current_line=$('<p id="row_1" class="current_line">')
			$('#term').html(current_line)
			maxRow=1 // I already created one
		}
		else{
			current_line=l
			maxRow=$('#term p').length
		}
	}
	else if (t=='1'){ // from cursor to start of screen
		var sp=getCurrentColumnSpan()
		if (!sp)
			return
		sp[0].html(substr(sp[0].html(), sp[1])) // on current span, remove leading text
		
		// prev spans
		var n=sp[0].prev()
		while (n.length!=0){
			var nn=n.prev()
			n.remove()
			n=nn
		}
		// prev lines
		var n=current_line.prev()
		while (n.length!=0){
			var nn=n.prev()
			n.remove()
			n=nn
		}
	}
	else if (t=='2'){
		current_line=$('<p id="row_1" class="current_line">')
		$('#term').html(current_line)
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
	if (current_line.attr('id')=='row_'+rn){
		return
	}
	current_line.removeClass('current_line')
	dataAddEasy=false
	/*
	alert(maxRow)
	alert(rn)
	*/
	while (rn>maxRow){
		dataAddEasy=true
		maxRow++;

		var p=$('<p>').attr('id','row_'+maxRow)
		changedClass=false
		$('#term').append(p)
	}
	
	var r=$('#row_'+rn)
	r.addClass('current_line')
	current_line=r
	posRow=rn
}


/// Goes to the given column
gotoCol = function(cn){
	var l=elength(current_line.text())+1
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

/// Moves down some positions
scrollDown = function(st){
	var d
	if (!st)
		d=1
	else
		d=Number(st)
	setPosition(posRow+d, posColumn)
}

/// Moves down some positions
scrollUp = function(st){
	var u
	if (!st)
		u=1
	else
		u=Number(st)
	setPosition(posRow-u, posColumn)
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

mouseSupportOn = function(){
	mouseSupport=true
	showMsg('Enabled mouse support')
}

mouseSupportOff = function(){
	mouseSupport=false
	showMsg('Disabled mouse support')
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
		current_line.find('span').remove()
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

/// Writes ' ' for some time
eraseCharacters = function(n){
	if (n)
		n=Number(n)
	else
		n=1
	var s=''
	for (var i=0;i<n;i++){
		s+='&nbsp;'
	}
	addText(s,n);
}


/**
 * @return returns the current span and position inside span.
 */
getCurrentColumnSpan = function(){
	var i=0
	var m=posColumn
	var ret=false
	current_line.find('> span').each(function(){
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
/// From here until \007 it is written title. Prepare parser.
prepareForTitle = function(){
	parserStatus='title'
}

prepareForOtermPosition = function(){
	parserStatus='oterm_position'
}

prepareForOtermURL = function(){
  parserStatus='oterm_url'
}


/// Sets the title. 
setTitle = function(title){
	$.post('title',{title:title})
	
	document.title=title
}

/// Oterm custom command, set the read position
otermReadPos = function(pos){
	readDataPos=Number(pos)
}

/// Oterm custom command, set the url
otermSetURL = function(url){
  $('#header #url').attr('href',url).text(url)
}

