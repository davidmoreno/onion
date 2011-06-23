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

/// A list of processed received status changes
statusLog=[]

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

/// Timeout to show a message
msgTimeout=0
/// Status of the parser: 0 normal, 1 set status type 1, 2 set status type 2.
parserStatus=0

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
/// Mouse support
mouseSupport=false

/// Current line, instead of always search for it, here it is. always.
current_line=$('')

/// Current screen size
nrRows=0
nrCols=0

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

	if (current_line.length==0){ // I need to write somewhere
		gotoRow(posRow) // ensure it creates the row
		setPosition(posRow, posColumn)
	}

	if (current_line.find('span').length==0) // I need to write somewhere
		current_line.append($('<span>'))
		
	easyAdd = function(){
		if (changedClass)
			current_line.append( $('<span>').html(text).addClass(currentClass) ).showAndScroll()
		else{
			var l=current_line.find('span:last')
			l.html( $('<span>').html(l.html()+text) ).showAndScroll()
		}
	}
		
	if (dataAddEasy){
		easyAdd()
	}
	else{
		var sp=getCurrentColumnSpan()
		if (!sp){
			easyAdd()
		}
		else{
			var head_pos=sp[1]
			var head=sp[0]


			var my_length=elength(text)
			var head_text=head.text()
				
			var insert_point // Where to insert remaining data, position in line
			if (editModeInsert)
				insert_point=posColumn
			else
				insert_point=posColumn+my_length


// options
// 1. same class, fits inside
// 2. diferent class, fits inside
// 3. same class, do not fit inside. fits with dest class
// 3. same class, do not fit inside. do not fits with dest class
// 4. diferent class, do not fit inside. fits with dest class
// 4. diferent class, do not fit inside. do not fits with dest class

			var head_end=head_text.length+posColumn-head_pos
			if (insert_point<=head_end){ // fits inside
				var insert_point_at_head=insert_point-(head_end-head_text.length)
				// first remove the not wanted anymore characters
				var p1=substr(head_text, 0, head_pos)
				var p2=substr(head_text, insert_point_at_head)
				if (head.attr('class')==currentClass){ // same class, just concat
					head.html(p1 + text + p2)
				}
				else{ // diferent class, make 3 spans
					head.html(p1)
					var me=$('<span>').html(text).addClass(currentClass)
					head.after(me)
					var tail=$('<span>').html(p2).addClass(head.attr('class'))
					me.after(tail)
				}
			}
			else{ // do not fit inside
				// first remove unnecesary spans and leading and tailing text, to leave a blank of tlength spaces
				head.html(substr(head.html(),0,head_pos))
				var removedChars=head_text.length-head_pos
				var n=head.next()
				while (removedChars+n.text().length<my_length){ // remove the middle spans
					if (n.length==0)
						break
					removedChars+=n.text().length
					var nn=n.next()
					n.remove()
					n=nn
				}
				var tail=n
				// and remove tailing leftovers. 
				if (tail.length!=0)
					tail.html(substr(tail.html(), my_length-removedChars))
				
				/// if fits with sp class
				var me=head
				if (currentClass==head.attr('class')){
					head.html(head.html()+text)
				}
				else{
					me=$('<span>').html(text).addClass(currentClass)
					head.after(me)
				}
				// if me has same class as n, join us
				if (tail.length!=0 && tail.attr('class')==currentClass){
					me.html(me.html()+tail.html())
					tail.remove()
				}
			}
		}
	}
	posColumn+=length
	
	updateCursor()
}

cacheSendKeys=''
onpetitionIn=false // no two petitions at the same time.
onpetitionOut=false // no two petitions at the same time.

/**
 * @short Request new data, and do the timeout for next petition
 *
 * Data send is serialized: only one for each direction (in/out) petition can be on the air. If a new petition is done, then 
 * it must wait until the reponse of latest arrives, and then make a new one. This helps a lot the interactivity, and solves
 * serialization problems.
 */
requestNewData = function(keyvalue){
	if (keyvalue)
		cacheSendKeys+=keyvalue
	if (!onpetitionIn && cacheSendKeys){
		onpetitionIn=true
		$.post('in',{type:cacheSendKeys}, function(){ onpetitionIn=false; requestNewData(); })
		cacheSendKeys=''
	}
	if (!onpetitionOut){
		onpetitionOut=true
		$.get('out',updateRequestData,'plain')
	}
}

/// Sets the result of data load, and set a new petition for later.
updateRequestData = function(text){
	updateData(text)
	
	onpetitionOut=false
	if (text!="")
		requestNewData()
	else
		showMsg("Program exited.");
}

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

/// Updates the geometry, and sends a message if necessary.
updateGeometry = function(){
	var t=$(document)
	var r = parseInt(t.height()/charHeight)-5
	var c = parseInt(t.width()/charWidth)-3
	if (r!=nrRows || c!=nrCols){
		nrRows=r
		nrCols=c
		showMsg('New geometry is '+nrRows+' rows, '+nrCols+' columns.')
		$.post('resize',{width:nrRows, height:nrCols})
	}
}

/// From here until \007 it is written title. Prepare parser.
prepareForTitle = function(){
	parserStatus='title'
}

/// Sets the title. 
setTitle = function(title){
	$.post('title',{title:title})
	
	document.title=title
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
