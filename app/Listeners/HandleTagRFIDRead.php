<?php

namespace App\Listeners;

use App\Events\TagRFIDReadEvent;
use App\Events\UnlockEvent;
use App\Models\ActivationRecord;
use App\Models\User;
use Illuminate\Contracts\Queue\ShouldQueue;
use Illuminate\Queue\InteractsWithQueue;

class HandleTagRFIDRead
{
    /**
     * Create the event listener.
     */
    public function __construct()
    {
        //
    }

    /**
     * Handle the event.
     */
    public function handle(TagRFIDReadEvent $event): void
    {
        $user = User::where('uid', $event->tagRFID)->first();

        if (!$user) {
            return;
        }

        $activationRecord = new ActivationRecord();
        $activationRecord->user_id = $user->id;
        $activationRecord->date = now()->toDateString();
        $activationRecord->time = now()->toTimeString();

        UnlockEvent::dispatch();
    }
}
