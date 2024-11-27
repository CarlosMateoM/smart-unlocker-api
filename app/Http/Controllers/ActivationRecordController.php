<?php

namespace App\Http\Controllers;

use App\Events\TagRFIDReadEvent;
use App\Events\UnlockEvent;
use App\Http\Requests\StoreActivationRecordRequest;
use App\Http\Requests\UpdateActivationRecordRequest;
use App\Http\Resources\ActivationRecordResource;
use App\Models\ActivationRecord;
use App\Models\User;
use Illuminate\Http\Request;
use Spatie\QueryBuilder\AllowedFilter;
use Spatie\QueryBuilder\QueryBuilder;

class ActivationRecordController extends Controller
{
    /**
     * Display a listing of the resource.
     */
    public function index(Request $request)
    {
        $user = $request->user();

        $activationRecords = QueryBuilder::for(ActivationRecord::class)
            ->allowedFilters([
                AllowedFilter::exact('user_id'),
                'user.name',
                AllowedFilter::callback('date', function($query, $date) {
                    if(is_array($date)) {
                        $query->whereBetween('date', $date);
                        return;
                    }
                    $query->where('date', $date);
                }),
                'time'
            ])
            ->with('user')
            ->orderBy('date', 'desc')
            ->orderBy('time', 'desc');

        if($user->role === 'user') {
            $activationRecords->where('user_id', $user->id);
        }

        $activationRecords = $activationRecords->paginate(15);

        return ActivationRecordResource::collection($activationRecords)->resource;
    }

    /**
     * Store a newly created resource in storage.
     */
    public function store(StoreActivationRecordRequest $request)
    {
        $user = User::where('uid', $request->input('uid'))->first();

        if(!$user) {

            TagRFIDReadEvent::dispatch($request->input('uid'));

            return response()->json([
                'should_unlock' => false,
                'message'       => 'The user is not registered',
            ], 404);
        }

        if (! $request->user()->is_enable) {
            return response()->json([
                'status' => 'user_is_not_enabled',
                'message' => 'User is not enabled'
            ], 403);
        }

        $activationRecord = new ActivationRecord();
        $activationRecord->user_id  = $user->id;
        $activationRecord->date     = now()->toDateString();
        $activationRecord->time     = now()->toTimeString();
        
        $activationRecord->save();

        UnlockEvent::dispatch();

        $activationRecord->load('user');

        return response()->json([
            'should_unlock' => true,
            'message'       => 'The user is registered',
        ], 201);
    }

    /**
     * Display the specified resource.
     */
    public function show(ActivationRecord $activationRecord)
    {
        //
    }

    /**
     * Update the specified resource in storage.
     */
    public function update(UpdateActivationRecordRequest $request, ActivationRecord $activationRecord)
    {
        //
    }

    /**
     * Remove the specified resource from storage.
     */
    public function destroy(ActivationRecord $activationRecord)
    {
        //
    }
}
