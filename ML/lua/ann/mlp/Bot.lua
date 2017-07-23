local Bot={};

Bot.actions={};
Bot.actions[GameAgentAction.ATTACK]=1;
Bot.actions[GameAgentAction.IDLE]=2;
Bot.actions[GameAgentAction.APPROACH]=3;
Bot.actions[GameAgentAction.WANDER]=4;
Bot.actions[GameAgentAction.ESCAPE]=5;

Bot.err_threshold=\param{MinErrorThreshold};
Bot.max_epoches=\param{MaxEpoches};
Bot.stagnation_epoches=\param{StagnationEpoches};
Bot.training_error=100000;
Bot.min_improvement=\param{StagnationThreshold};

function Bot.initialize(agent)
	--agent:addEnemy("PreyBot");
	--agent:addEnemy("PredatorBot");
	agent:setWalkSpeed(40);
	agent:setSenseRange(250);
	agent:setLife(300);
	
	local agentId=agent:getAgentId();
	
	Bot[agentId]={};
	Bot[agentId].brain=Bot.createMLP(agent:getScriptClassPath());
end

function Bot.createMLP(scriptClassPath)
	local brain={};	
	
	local MLPFactory=dofile(scriptClassPath .. "\\MLP.lua");
	local brain=MLPFactory.create(\param{LearningRate});
	brain:addLayer(4); --input layer --Xianshun: I change it to 4...
	\begin{template}[HiddenLayers]
	brain:addLayer(\param{LayerNeuronCount}); --hidden layer
	\end{template}
	brain:addLayer(5); --output layer	

	if fileExists(scriptClassPath .. "\\Saved.lua") then
		brain:load(scriptClassPath .. "\\Saved.lua");
	end
	
	return brain;
end

function Bot.getInput(entity)
	local inputs={};
	
	inputs[1]=entity:isTargetAttackable();
	inputs[2]=entity:getSightedAttackerCount();
	inputs[3]=entity:getTargetRelativeDistance();
	inputs[4]=entity:getTargetRelativeLife();
	--inputs[5]=entity:getGun():getBulletCount();
	--inputs[6]=entity:getLife();
	--inputs[7]=entity:getScore();
	--Xianshun: I intentionally remove the last 3 input variables...
	
	return inputs;
end

function Bot.getOutput(entity)
	local outputs={};
	
	outputs[1]=0;
	outputs[2]=0;
	outputs[3]=0;
	outputs[4]=0;
	outputs[5]=0;
	
	outputs[entity:getCurrentAction()+1]=1;
	
	-- since the value for current action is given by
	-- GameAgentAction.ATTACK=0;
	-- GameAgentAction.IDLE=1;
	-- GameAgentAction.APPROACH=2;
	-- GameAgentAction.WANDER=3;
	-- GameAgentAction.ESCAPE=4;
		
	return outputs;
end

function Bot.train(agent)
	local agentId=agent:getAgentId();
	local records=dofile(agent:getScriptClassPath() .. "\\data.lua");
	
	local brain=Bot[agentId].brain;
	local err=0;
	
	showConsole(true);
	
	local minError=10000000000;
	local stagnationCount=0;
	for epoch = 1, Bot.max_epoches do
		err=0;
		for recordIndex = 1, (# records) do
			local inputs=Bot.getInput(records[recordIndex]);
			local outputs=Bot.getOutput(records[recordIndex]);		
		
			brain:forwardProp(inputs);
			brain:backwardProp(outputs);
			err=err + brain:getMSE(outputs);
		end
		print2Console("> epoch: " .. epoch .. " error: " .. err);
		repaint();
		if minError > err then
			minError = err;
			brain:saveWeights();
		else
			stagnationCount=stagnationCount + 1;
		end
		
		if stagnationCount > Bot.stagnation_epoches then
			brain:randomizeWeights();
			stagnationCount=0;
		end
	end
	
	brain:loadWeights();
	
	showConsole(false);
	brain:save(agent:getScriptClassPath() .. "\\Saved.lua");
	
	Bot.training_error=err;
	alert("Training Completed with errors " .. err, "Training Completed");
end

function Bot.think(agent)
	local agentId=agent:getAgentId();
	local brain=Bot[agentId].brain;
	--apply inputs to the neural network
	
	local inputs=Bot.getInput(agent);
	
	--compute outputs using neural network
	local outputs=brain:forwardProp(inputs);
	
	--convert neural network output into bot action
	local firingIndex=-1;
	local firingDegree=-100000;
	for i=1, 5 do
		if outputs[i] > firingDegree then
			firingIndex=i;
			firingDegree=outputs[i];
		end
	end
	firingIndex=firingIndex-1;
	
	if GameAgentAction.ATTACK == firingIndex then
		agent:attack();
	elseif GameAgentAction.APPROACH == firingIndex then
		agent:approach();
	elseif GameAgentAction.ESCAPE == firingIndex then
		agent:escape();
	elseif GameAgentAction.WANDER == firingIndex then
		agent:wander();
	elseif GameAgentAction.IDLE == firingIndex then
		agent:idle();
	end
end

function Bot.uploadConfig(agent)
	local agentId=agent:getAgentId();
	local brain=Bot[agentId].brain;
	local hiddenLayers=brain:getLayerCount() - 2;
	
	httpAddField("HiddenLayers", "" .. hiddenLayers);
	httpAddField("learningRate", "" .. (brain.learningRate));
	httpAddField("trainingError", "" .. (Bot.training_error));
	if hiddenLayers > 0 then
		for i=1, hiddenLayers do
			httpAddField("neuronCountInHiddenLayer" .. i, "" .. brain.network[i+1].neuronCount);
		end
	end
end

return Bot;
