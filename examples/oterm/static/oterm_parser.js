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


/// All data readen so far. For debugging. FIXME
allData=''

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
				parserStatus=0
				try{
					setStatus(str)
				}
				catch(e){
					showMsg("Exception trying to set a status!");
				}
				clear()
			}
		}
		else if (parserStatus==3){
			if (c==';'){
				parserStatus=0
				setStatusType2(str)
				clear()
			}
			else
				str+=c
		}
		else if (parserStatus=='title'){
			if (c=='\007'){
				setTitle(str)
				clear()
				parserStatus=0
			}
			else
				str+=c
		}
		else if (parserStatus=='oterm_position'){
			if (c==';'){
				otermReadPos(str)
				clear()
				parserStatus=0
			}
			else
				str+=c
		}
    else if (parserStatus=='oterm_url'){
      if (c==';'){
        otermSetURL(str)
        clear()
        parserStatus=0
      }
      else
        str+=c
    }

	}
	addText(str,length)
}

/// Debugging help
showHex = function(){
	var s=''
	for (var i in allData){
		var v=allData.charCodeAt(i)
		s+=allData[i]+" 0"+v.toString(8)+"\n"
	}
	return s
}


