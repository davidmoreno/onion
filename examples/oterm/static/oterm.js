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
readDataPos=0

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
		$.get('out',{pos:readDataPos},updateRequestData,'text')
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



/// Shows a message on the window about current status change.
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

/// Prepares basic status of the document: keydown are processed, and starts the updatedata.
$(document).ready(function(){
	var t=$('#term span')
	charWidth=(t.width()/t.text().length)
	charHeight=t.height()-3
	
	updateCursor()
	
	$('#term').html('')
	newLine()
	
	// Ask for stored buffer data
	onpetitionOut=true
	$.get('out?initial',updateRequestData, 'text')
	
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
