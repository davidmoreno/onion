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
 * Input from the browser that will be sned to the remote terminal.
 */

/// Parses key presses.
keypress = function(event){
	var keyCode=event.keyCode
	var charCode=event.charCode
	event.preventDefault()
	if (event.ctrlKey)
		return
	//showMsg('Key pressed '+keyCode+" "+charCode)
	var keyValue=keyValue=String.fromCharCode(charCode)
	
	/// If any text, do the petition.
	requestNewData(keyValue)
}

/// Some keys must be prevented to act, like FX
keydown = function(event){
	var keyCode=event.keyCode
	var keyValue=''
	//showMsg('Key down '+keyCode)
	
	if (keyCode in keyCodesToValues){
		keyValue=keyCodesToValues[keyCode]
	}
	else if (event.ctrlKey && keyCode){
		keyValue=keyCodesToValuesControl[String.fromCharCode(keyCode)]
		//showMsg('Sent control '+String.fromCharCode(keyCode))
	}
	
	if (keyValue!='' && keyValue!=' '){
		event.preventDefault()
		requestNewData(keyValue)
	}
}

$(document).ready(function(){
	$(document).keydown(keydown)
	$(document).keypress(keypress)
})
