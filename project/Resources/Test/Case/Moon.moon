Dorothy!

TestBase = require "Test.Case.TestBase"
Class,property = unpack require "Lib.class"

Class TestBase,
	__init: =>
		{:width,:height} = CCDirector.winSize

		model = with oModel "ActionEditor/Model/Output/xiaoli.model"
			.look = "happy"
			.loop = true
			.position = oVec2 width*0.5, height*0.5
			\play "walk"
			\slot "AnimationEnd",(name)->print name

		layer = with CCLayer!
			.anchor = oVec2.zero
			.touchEnabled = true
			\slot "TouchMoved",(touch)->model.position += touch.delta
			\addChild model

		@addChild layer

