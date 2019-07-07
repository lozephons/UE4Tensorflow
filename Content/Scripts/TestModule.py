#converted for ue4 use from
#https://github.com/tensorflow/docs/blob/master/site/en/tutorials/_index.ipynb

import math
import random
import sys
from time import sleep

import numpy as np
#import tensorflow as tf
import tensorflow as tf

import unreal_engine as ue

#additional includes
#from tensorflow.python.keras import backend as K	#to ensure things work well with multi-threading
#import numpy as np   	#for reshaping input
#import operator      	#used for getting max prediction from 1x10 output array

class Model():
	def __init__(self, num_states, num_actions, batch_size):
		self._num_states = num_states
		self._num_actions = num_actions
		self._batch_size = batch_size
		
		self._states = None
		self._actions = None

		self._logits = None
		self._optimizer = None
		self._var_init = None

		self._define_model()

	def _define_model(self):
		self._states = tf.compat.v1.placeholder(shape=[None, self._num_states], dtype=tf.float32)
		self._q_s_a = tf.compat.v1.placeholder(shape=[None, self._num_actions], dtype=tf.float32)
		
		fc1 = tf.compat.v1.keras.layers.Dense(100, activation = tf.nn.relu)(self._states)
		fc2 = tf.compat.v1.keras.layers.Dense(100, activation = tf.nn.relu)(fc1)
		#fc3 = tf.compat.v1.keras.layers.Dense(100, activation = tf.nn.relu)(fc2)
		self._logits = tf.compat.v1.keras.layers.Dense(self._num_actions)(fc2)
		loss = tf.compat.v1.losses.mean_squared_error(self._q_s_a, self._logits)
		self._optimizer = tf.compat.v1.train.AdamOptimizer().minimize(loss)
		self._var_init = tf.compat.v1.global_variables_initializer()

	def predict_one(self, state, sess):
		return sess.run(self._logits, feed_dict = {self._states:state.reshape(1, self._num_states)})

	def predict_batch(self, states, sess):
		return sess.run(self._logits, feed_dict = {self._states:states})
	
	def train_batch(self, sess, x_batch, y_batch):
		sess.run(self._optimizer, feed_dict = {self._states:x_batch, self._q_s_a: y_batch})

class Memory:
	def __init__(self, max_memory):
		self._max_memory = max_memory
		self._samples = []

	def add_sample(self, sample):
		self._samples.append(sample)
		if len(self._samples) > self._max_memory:
			self._samples.pop(0)

	def sample(self, no_samples):
		if no_samples > len(self._samples):
			return random.sample(self._samples, len(self._samples))
		else:
			return random.sample(self._samples, no_samples)

class GameRunner:
	def __init__(self, sess, model, memory, max_eps, min_eps, decay, gamma):
		self._sess = sess
		self._model = model
		self._memory = memory
		#self._render = render
		self._max_eps = max_eps
		self._min_eps = min_eps
		self._decay = decay
		self._eps = self._max_eps
		self._steps = 0
		self._reward_store = []
		self._max_dist_store = []
		self._gamma = gamma

	def resetState(self, state):
		self._state = state
		self._tot_reward = 0
		self._max_dist = 0

	def getaction(self, bUseRandom):
		self._action = self._choose_action(self._state, bUseRandom)
		return self._action

	def step(self, dist, next_state, reward, done):
		if done:
			next_state = None

		self._memory.add_sample((self._state, self._action, reward, next_state))
		self._replay()

		self._steps += 1
		self._eps = self._min_eps + (self._max_eps - self._min_eps) * math.exp(-self._decay * self._steps)

		self._state = next_state
		self._tot_reward += reward

		if done:
			ue.log_warning("Goal! - " + str(self._tot_reward))
			self._reward_store.append(self._tot_reward)
			self._max_dist_store.append(dist)
			return True
		
		return False

	def _choose_action(self, state, bUseRandom):
		if bUseRandom == False and random.random() < self._eps:
			return random.randint(0, self._model._num_actions - 1)
		else:
			return np.argmax(self._model.predict_one(state, self._sess))

	def _replay(self):
		batch = self._memory.sample(self._model._batch_size)
		states = np.array([val[0] for val in batch])
		next_states = np.array([(np.zeros(self._model._num_states) if val[3] is None else val[3]) for val in batch])

		q_s_a = self._model.predict_batch(states, self._sess)
		q_s_a_d = self._model.predict_batch(next_states, self._sess)

		x = np.zeros((len(batch), self._model._num_states))
		y = np.zeros((len(batch), self._model._num_actions))

		for i, b in enumerate(batch):
			state, action, reward, next_state = b[0], b[1], b[2], b[3]

			current_q = q_s_a[i]
			if next_state is None:
				current_q[action] = reward
			else:
				current_q[action] = reward + self._gamma * np.amax(q_s_a_d[i])
			
			x[i] = state
			y[i] = current_q
		
		self._model.train_batch(self._sess, x, y)

class Main():
	def Setup(self, num_states, num_actions, batch_size):
		self._model = Model(num_states, num_actions, batch_size)
		self._memory = Memory(100000)

		self._session = tf.compat.v1.Session()
		self._session.run(self._model._var_init)
		
		MAX_EPSILON = 0.9
		MIN_EPSILON = 0.01
		LAMBDA = 0.001
		GAMMA = 0.2
		self._gamerunner = GameRunner(self._session, self._model, self._memory, MAX_EPSILON, MIN_EPSILON, LAMBDA, GAMMA)

		ue.log_warning("Setup Ended")

	def RunBegin(self, state):
		state = np.reshape(state, len(state))
		self._gamerunner.resetState(state)
		ue.log_warning("RunBegin - " + str(len(state)))

	def GetNextAction(self, bUseRandom):
		return self._gamerunner.getaction(bUseRandom)

	def Run(self, dist, next_state, reward, done):
		#ue.log_warning("Run")
		next_state = np.reshape(next_state, len(next_state))
		return self._gamerunner.step(dist, next_state, reward, done)

	def test(self, a):
		#ue.log_warning("Test " + str(len(a)) + " " + str(a[random.randrange(0, 1000)]))
		a = 1
